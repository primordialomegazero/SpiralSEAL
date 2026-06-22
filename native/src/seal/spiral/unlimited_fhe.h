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

namespace seal {
namespace spiral {

class UnlimitedFHE {
public:
    static constexpr double PHI = 1.6180339887498948482;
    static constexpr double PHI_INV = 0.6180339887498948482;
    static constexpr double LYAPUNOV = 0.48121182505960347;

    struct Config {
        int fractal_depth;
        double target_noise;
        bool dynamic_modulus;
        bool true_noise_reset;
        bool auto_enc_zero;
        bool deep_correction;
        Config() : fractal_depth(7), target_noise(40.0), dynamic_modulus(true),
                   true_noise_reset(true), auto_enc_zero(true), deep_correction(true) {}
    };

    struct Stats {
        double initial_noise = 0;
        double final_noise = 0;
        int layers_converged = 0;
        double time_ms = 0;
        bool noise_reset_achieved = false;
        bool value_preserved = false;
    };

    UnlimitedFHE(const SEALContext &context,
                 const SecretKey &sk,
                 const Config &config = Config());

    bool bootstrap(Ciphertext &encrypted, Stats *stats = nullptr);

private:
    const SEALContext &context_;
    SecretKey sk_;
    Config config_;
    BatchEncoder encoder_;
    Ciphertext enc_zero_;
    
    void dynamic_modulus_scale(Ciphertext &ct, uint64_t original_value);
    void lyapunov_noise_reset(Ciphertext &ct);
    void homomorphic_enc_zero(Ciphertext &enc_zero_out);
    void deep_phi_correction(std::vector<Ciphertext> &layers, const Ciphertext &anchor);
};

} // namespace spiral
} // namespace seal
