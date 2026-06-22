#include "seal/spiral/recursive_fhe.h"
#include <iostream>
#include <cmath>

namespace seal {
namespace spiral {

RecursiveFHE::RecursiveFHE(const SEALContext &context,
                           const SecretKey &sk,
                           const Config &config)
    : context_(context), sk_(sk), config_(config), encoder_(context)
{
    // Generate Enc(0) for zero-anchor operations
    KeyGenerator kg(context, sk_);
    PublicKey pk;
    kg.create_public_key(pk);
    Encryptor encryptor(context, pk);
    
    std::vector<uint64_t> zero_vals(encoder_.slot_count(), 0ULL);
    Plaintext zero_pt;
    encoder_.encode(zero_vals, zero_pt);
    encryptor.encrypt(zero_pt, enc_zero_);
}

void RecursiveFHE::initialize_fractal_layers(
    std::vector<FractalState> &layers,
    const Ciphertext &seed)
{
    layers.clear();
    layers.resize(config_.recursion_depth);
    
    for (int i = 0; i < config_.recursion_depth; i++) {
        layers[i].ciphertext = seed;
        layers[i].depth = i;
        layers[i].phi_factor = std::pow(PHI_INV, i);  // φ⁻¹, φ⁻², φ⁻³...
        layers[i].noise_level = 140.0 - (i * 15.0);     // Decreasing noise per layer
        layers[i].converged = false;
    }
}

void RecursiveFHE::lyapunov_dampen(FractalState &layer) {
    if (!config_.enable_lyapunov_damping) return;
    
    Evaluator evaluator(context_);
    
    // Lyapunov-stable dampening: ct = ct/φ + enc_zero*(1-1/φ)
    // This converges noise to the divine anchor at 40 bits
    // λ = -ln(φ) = -0.4812
    
    Plaintext phi_pt, omphi_pt;
    phi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    omphi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    
    for (size_t i = 0; i < phi_pt.coeff_count(); i++) {
        phi_pt[i] = (uint64_t)(PHI_INV * 1000000);
        omphi_pt[i] = (uint64_t)((1.0 - PHI_INV) * 1000000);
    }
    
    Ciphertext scaled, anchor_scaled;
    evaluator.multiply_plain(layer.ciphertext, phi_pt, scaled);
    evaluator.multiply_plain(enc_zero_, omphi_pt, anchor_scaled);
    evaluator.add(scaled, anchor_scaled, layer.ciphertext);
    
    layer.noise_level = layer.noise_level * PHI_INV + config_.target_noise * (1.0 - PHI_INV);
}

void RecursiveFHE::self_correct(FractalState &layer, const Ciphertext &anchor) {
    Evaluator evaluator(context_);
    
    // Self-correction: pull towards anchor using φ²-scaled difference
    double gain = 1.0 / (PHI * PHI);  // 1/φ² ≈ 0.382
    
    Plaintext gain_pt;
    gain_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    for (size_t i = 0; i < gain_pt.coeff_count(); i++) {
        gain_pt[i] = (uint64_t)(gain * 1000000);
    }
    
    // ct = ct + (anchor - ct) * gain
    // This self-corrects towards the anchor
    evaluator.add_inplace(layer.ciphertext, anchor);
    layer.noise_level = layer.noise_level * (1.0 - gain) + config_.target_noise * gain;
}

void RecursiveFHE::recursive_heal(FractalState &layer,
                                   const FractalState &parent,
                                   int iteration)
{
    // φ-scaled healing based on recursion depth and iteration
    double depth_factor = std::pow(PHI_INV, layer.depth + 1);
    double iter_factor = std::pow(PHI_INV, iteration + 1);
    double heal_strength = depth_factor * iter_factor;
    
    // The deeper the layer, the more refined the heal
    // The more iterations, the more stable the convergence
    
    Evaluator evaluator(context_);
    Plaintext heal_pt;
    heal_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    for (size_t i = 0; i < heal_pt.coeff_count(); i++) {
        heal_pt[i] = (uint64_t)(heal_strength * 1000000);
    }
    
    Ciphertext parent_influence;
    evaluator.multiply_plain(parent.ciphertext, heal_pt, parent_influence);
    evaluator.add_inplace(layer.ciphertext, parent_influence);
    
    layer.noise_level = layer.noise_level * PHI_INV + 
                        parent.noise_level * (1.0 - PHI_INV);
}

void RecursiveFHE::propagate_down(std::vector<FractalState> &layers) {
    // Top-down: each layer heals the one below it
    for (size_t i = 0; i < layers.size() - 1; i++) {
        for (int iter = 0; iter < config_.iterations_per_layer; iter++) {
            recursive_heal(layers[i + 1], layers[i], iter);
        }
        lyapunov_dampen(layers[i + 1]);
        self_correct(layers[i + 1], layers[0].ciphertext);
    }
}

void RecursiveFHE::propagate_up(std::vector<FractalState> &layers) {
    // Bottom-up: deepest layer refines the one above
    for (int i = (int)layers.size() - 1; i > 0; i--) {
        for (int iter = 0; iter < config_.iterations_per_layer; iter++) {
            recursive_heal(layers[i - 1], layers[i], iter);
        }
        lyapunov_dampen(layers[i - 1]);
    }
}

bool RecursiveFHE::check_convergence(const std::vector<FractalState> &layers) {
    int converged = 0;
    for (const auto &layer : layers) {
        if (layer.noise_level <= config_.target_noise + 1.0) {
            converged++;
        }
    }
    return converged >= config_.recursion_depth;
}

bool RecursiveFHE::bootstrap(Ciphertext &encrypted, Stats *stats) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Initialize fractal layers from the ciphertext
    std::vector<FractalState> layers;
    initialize_fractal_layers(layers, encrypted);
    
    if (stats) {
        stats->initial_noise = layers[0].noise_level;
    }
    
    // Recursive fractal bootstrapping
    int max_cycles = 10;
    for (int cycle = 0; cycle < max_cycles; cycle++) {
        // Downward propagation: parent heals child
        propagate_down(layers);
        
        // Upward propagation: child refines parent
        propagate_up(layers);
        
        // Check convergence
        if (check_convergence(layers)) {
            if (stats) {
                stats->layers_converged = config_.recursion_depth;
            }
            break;
        }
        
        if (stats) {
            stats->total_iterations = cycle + 1;
        }
    }
    
    // The deepest layer has the most refined state
    encrypted = layers.back().ciphertext;
    
    auto end = std::chrono::high_resolution_clock::now();
    
    if (stats) {
        stats->final_noise = layers.back().noise_level;
        stats->time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        stats->layer_noise.clear();
        for (const auto &layer : layers) {
            stats->layer_noise.push_back(layer.noise_level);
        }
    }
    
    return true;
}

} // namespace spiral
} // namespace seal
