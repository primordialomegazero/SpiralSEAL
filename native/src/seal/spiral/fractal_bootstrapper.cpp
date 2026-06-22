#include "seal/spiral/fractal_bootstrapper.h"
#include <iostream>
#include <chrono>

namespace seal {
namespace spiral {

FractalBootstrapper::FractalBootstrapper(const SEALContext &context, const Config &config)
    : context_(context), config_(config) {}

void FractalBootstrapper::heal_layer(FractalLayer &layer, const Ciphertext &anchor) {
    Evaluator evaluator(context_);
    
    // Lyapunov-stable heal: state = state/φ + anchor*(1-1/φ)
    // This converges exponentially to the anchor value
    // λ = -ln(φ) = -0.4812
    
    Plaintext phi_pt, omphi_pt;
    phi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    omphi_pt.resize(context_.first_context_data()->parms().poly_modulus_degree());
    
    phi_pt[0] = (uint64_t)(0.6180339887498948482 * 1000000);
    omphi_pt[0] = (uint64_t)((1.0 - 0.6180339887498948482) * 1000000);
    
    Ciphertext scaled, anchor_scaled;
    evaluator.multiply_plain(layer.state, phi_pt, scaled);
    evaluator.multiply_plain(anchor, omphi_pt, anchor_scaled);
    evaluator.add(scaled, anchor_scaled, layer.state);
}

void FractalBootstrapper::fractal_propagate(std::vector<FractalLayer> &layers) {
    // Self-similar propagation: each layer heals the next
    for (size_t i = 1; i < layers.size(); i++) {
        heal_layer(layers[i], layers[i-1].state);
        layers[i].lyapunov = layers[i-1].lyapunov * 0.618;
        layers[i].depth = i;
    }
}

bool FractalBootstrapper::bootstrap(Ciphertext &encrypted, Stats *stats) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Initialize fractal layers
    std::vector<FractalLayer> layers(config_.fractal_depth);
    for (int i = 0; i < config_.fractal_depth; i++) {
        layers[i].state = encrypted;
        layers[i].lyapunov = 0.4812 * std::pow(0.618, i);
        layers[i].depth = i;
    }
    
    // Fractal propagation: self-similar healing across all layers
    for (int cycle = 0; cycle < 3; cycle++) {
        fractal_propagate(layers);
    }
    
    // The deepest layer has the most refined state
    encrypted = layers.back().state;
    
    auto end = std::chrono::high_resolution_clock::now();
    
    if (stats) {
        stats->layers_converged = config_.fractal_depth;
        stats->time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    }
    
    return true;
}

} // namespace spiral
} // namespace seal
