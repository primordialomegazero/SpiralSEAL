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

namespace seal {
namespace spiral {

class FractalBootstrapper {
public:
    static constexpr double PHI = 1.6180339887498948482;
    static constexpr int FRACTAL_DEPTH = 7;

    struct FractalLayer {
        Ciphertext state;
        double lyapunov = 0.4812;
        int depth = 0;
    };

    struct Config {
        int fractal_depth = FRACTAL_DEPTH;
        double target_noise = 40.0;
        bool self_healing = true;
    };

    struct Stats {
        int layers_converged = 0;
        double final_noise = 0;
        double time_ms = 0;
    };

    FractalBootstrapper(const SEALContext &context, const Config &config = Config());
    
    bool bootstrap(Ciphertext &encrypted, Stats *stats = nullptr);

private:
    const SEALContext &context_;
    Config config_;
    
    void heal_layer(FractalLayer &layer, const Ciphertext &anchor);
    void fractal_propagate(std::vector<FractalLayer> &layers);
};

} // namespace spiral
} // namespace seal
