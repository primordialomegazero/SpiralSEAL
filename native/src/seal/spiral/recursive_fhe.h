#pragma once

#include "seal/context.h"
#include "seal/evaluator.h"
#include "seal/encryptor.h"
#include "seal/decryptor.h"
#include "seal/batchencoder.h"
#include "seal/keygenerator.h"
#include "seal/ciphertext.h"
#include "seal/plaintext.h"
#include "seal/secretkey.h"
#include "seal/publickey.h"
#include <vector>
#include <cmath>
#include <chrono>
#include <functional>

namespace seal {
namespace spiral {

class RecursiveFHE {
public:
    static constexpr double PHI = 1.6180339887498948482;
    static constexpr double PHI_INV = 0.6180339887498948482;
    static constexpr int MAX_DEPTH = 7;
    static constexpr double LYAPUNOV = 0.48121182505960347;

    struct FractalState {
        Ciphertext ciphertext;
        double noise_level = 140.0;
        double phi_factor = 1.0;
        int depth = 0;
        bool converged = false;
    };

    struct Config {
        int recursion_depth = MAX_DEPTH;
        double target_noise = 40.0;
        int iterations_per_layer = 3;
        bool enable_self_healing = true;
        bool enable_lyapunov_damping = true;
    };

    struct Stats {
        int total_iterations = 0;
        double initial_noise = 0;
        double final_noise = 0;
        int layers_converged = 0;
        double time_ms = 0;
        std::vector<double> layer_noise;
    };

    RecursiveFHE(const SEALContext &context,
                 const SecretKey &sk,
                 const Config &config = Config());

    bool bootstrap(Ciphertext &encrypted, Stats *stats = nullptr);

private:
    const SEALContext &context_;
    SecretKey sk_;
    Config config_;
    Ciphertext enc_zero_;
    BatchEncoder encoder_;

    void initialize_fractal_layers(std::vector<FractalState> &layers, 
                                    const Ciphertext &seed);
    void recursive_heal(FractalState &layer, const FractalState &parent, 
                        int iteration);
    void lyapunov_dampen(FractalState &layer);
    void self_correct(FractalState &layer, const Ciphertext &anchor);
    void propagate_down(std::vector<FractalState> &layers);
    void propagate_up(std::vector<FractalState> &layers);
    bool check_convergence(const std::vector<FractalState> &layers);
};

} // namespace spiral
} // namespace seal
