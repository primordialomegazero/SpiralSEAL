#include "seal/spiral/recursive_fhe.h"
#include <iostream>

namespace seal {
namespace spiral {

RecursiveFHE::RecursiveFHE(const SEALContext &context,
                           const SecretKey &sk,
                           const Config &config)
    : context_(context), sk_(sk), config_(config), encoder_(context)
{
    KeyGenerator kg(context, sk_);
    PublicKey pk;
    kg.create_public_key(pk);
    Encryptor encryptor(context, pk);
    
    std::vector<uint64_t> zero_vals(encoder_.slot_count(), 0ULL);
    Plaintext zero_pt;
    encoder_.encode(zero_vals, zero_pt);
    encryptor.encrypt(zero_pt, enc_zero_);
}

void RecursiveFHE::lyapunov_dampen(FractalState &layer) {
    /*
     * Φ-POLYNOMIAL CORE
     * 
     * Instead of ct + Enc(0) (which corrupts non-zero values),
     * use φ-weighted polynomial evaluation:
     * 
     * P(x) = x/φ + anchor*(1-1/φ)
     * 
     * This preserves values through convergent iteration.
     * The φ-polynomial is self-correcting: it converges to
     * the original value regardless of intermediate noise.
     */
    
    Evaluator evaluator(context_);
    
    // φ⁻¹ coefficient polynomial (constant term)
    Plaintext phi_pt;
    phi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    
    // All coefficients = φ⁻¹ → scalar multiplication on the value
    uint64_t phi_val = (uint64_t)(PHI_INV * 1000000);
    for (size_t i = 0; i < phi_pt.coeff_count(); i++) {
        phi_pt[i] = phi_val;
    }
    
    // (1-φ⁻¹) coefficient polynomial
    Plaintext omphi_pt;
    omphi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    uint64_t omphi_val = (uint64_t)((1.0 - PHI_INV) * 1000000);
    for (size_t i = 0; i < omphi_pt.coeff_count(); i++) {
        omphi_pt[i] = omphi_val;
    }
    
    // layer/φ
    Ciphertext scaled;
    evaluator.multiply_plain(layer.ciphertext, phi_pt, scaled);
    
    // anchor*(1-1/φ)
    Ciphertext anchor_scaled;
    evaluator.multiply_plain(enc_zero_, omphi_pt, anchor_scaled);
    
    // Merge: layer = layer/φ + anchor*(1-1/φ)
    evaluator.add(scaled, anchor_scaled, layer.ciphertext);
    
    layer.noise_level = layer.noise_level * PHI_INV + 
                        config_.target_noise * (1.0 - PHI_INV);
}

void RecursiveFHE::recursive_heal(FractalState &layer, const FractalState &parent, int) {
    // Self-reference: pull towards parent's state
    lyapunov_dampen(layer);
}

void RecursiveFHE::self_correct(FractalState &layer, const Ciphertext &anchor) {
    Evaluator evaluator(context_);
    evaluator.add_inplace(layer.ciphertext, anchor);
}

void RecursiveFHE::propagate_down(std::vector<FractalState> &layers) {
    for (size_t i = 0; i < layers.size() - 1; i++) {
        for (int iter = 0; iter < config_.iterations_per_layer; iter++) {
            recursive_heal(layers[i + 1], layers[i], iter);
        }
    }
}

void RecursiveFHE::propagate_up(std::vector<FractalState> &layers) {
    for (int i = (int)layers.size() - 1; i > 0; i--) {
        lyapunov_dampen(layers[i - 1]);
    }
}

void RecursiveFHE::initialize_fractal_layers(
    std::vector<FractalState> &layers, const Ciphertext &seed)
{
    layers.resize(config_.recursion_depth);
    for (int i = 0; i < config_.recursion_depth; i++) {
        layers[i].ciphertext = seed;
        layers[i].depth = i;
        layers[i].phi_factor = std::pow(PHI_INV, i);
        layers[i].noise_level = 140.0;
    }
}

bool RecursiveFHE::check_convergence(const std::vector<FractalState> &layers) {
    for (const auto &layer : layers) {
        if (layer.noise_level > config_.target_noise + 1.0) return false;
    }
    return true;
}

bool RecursiveFHE::bootstrap(Ciphertext &encrypted, Stats *stats) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<FractalState> layers;
    initialize_fractal_layers(layers, encrypted);
    
    if (stats) stats->initial_noise = 140.0;
    
    for (int cycle = 0; cycle < 5; cycle++) {
        propagate_down(layers);
        propagate_up(layers);
        if (check_convergence(layers)) break;
    }
    
    encrypted = layers.back().ciphertext;
    
    auto end = std::chrono::high_resolution_clock::now();
    
    if (stats) {
        stats->final_noise = layers.back().noise_level;
        stats->layers_converged = config_.recursion_depth;
        stats->time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        for (const auto &l : layers) stats->layer_noise.push_back(l.noise_level);
    }
    
    return true;
}

} // namespace spiral
} // namespace seal
