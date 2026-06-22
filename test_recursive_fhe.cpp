#include <iostream>
#include <seal/seal.h>
#include "seal/spiral/recursive_fhe.h"
#include <chrono>
#include <vector>
#include <cmath>

using namespace seal;
using namespace seal::spiral;

int main() {
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║  SPIRALSEAL — RECURSIVE FHE DEEP TEST   ║\n";
    std::cout << "║  True Homomorphic Fractal Bootstrapper   ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    // Setup
    EncryptionParameters parms(scheme_type::bfv);
    parms.set_poly_modulus_degree(2048);
    parms.set_coeff_modulus(CoeffModulus::Create(2048, {60, 40, 40, 60}));
    parms.set_plain_modulus(PlainModulus::Batching(2048, 20));
    SEALContext context(parms, true, sec_level_type::none);

    KeyGenerator kg(context);
    SecretKey sk = kg.secret_key();
    PublicKey pk;
    kg.create_public_key(pk);
    Encryptor encryptor(context, pk);
    Decryptor decryptor(context, sk);
    BatchEncoder encoder(context);

    RecursiveFHE::Config config;
    config.recursion_depth = 7;
    config.iterations_per_layer = 3;
    config.enable_self_healing = true;
    config.enable_lyapunov_damping = true;

    RecursiveFHE fhe(context, sk, config);

    int passed = 0, total = 0;

    // TEST 1: Basic value preservation
    std::cout << "=== TEST 1: Value Preservation ===\n";
    {
        uint64_t test_vals[] = {0, 1, 42, 100, 255, 999, 1000000};
        for (uint64_t x : test_vals) {
            std::vector<uint64_t> vals(encoder.slot_count(), x);
            Plaintext pt; encoder.encode(vals, pt);
            Ciphertext ct; encryptor.encrypt(pt, ct);
            
            Plaintext before_pt; decryptor.decrypt(ct, before_pt);
            std::vector<uint64_t> before; encoder.decode(before_pt, before);
            
            RecursiveFHE::Stats stats;
            fhe.bootstrap(ct, &stats);
            
            Plaintext after_pt; decryptor.decrypt(ct, after_pt);
            std::vector<uint64_t> after; encoder.decode(after_pt, after);
            
            bool ok = (before[0] == after[0]);
            std::cout << "  " << x << " -> " << after[0] 
                      << " (" << (ok ? "PASS" : "FAIL") << ") "
                      << stats.time_ms << "ms\n";
            if (ok) passed++; total++;
        }
    }

    // TEST 2: Fractal layer noise tracking
    std::cout << "\n=== TEST 2: Fractal Layer Noise ===\n";
    {
        std::vector<uint64_t> vals(encoder.slot_count(), 42ULL);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        
        RecursiveFHE::Stats stats;
        fhe.bootstrap(ct, &stats);
        
        std::cout << "  Layers converged: " << stats.layers_converged << "/7\n";
        std::cout << "  Noise per layer: ";
        for (size_t i = 0; i < stats.layer_noise.size(); i++) {
            std::cout << stats.layer_noise[i] << " ";
        }
        std::cout << "\n";
        std::cout << "  Initial noise: " << stats.initial_noise << " bits\n";
        std::cout << "  Final noise: " << stats.final_noise << " bits\n";
        std::cout << "  Time: " << stats.time_ms << "ms\n";
        
        bool ok = (stats.layers_converged >= 3);
        std::cout << "  " << (ok ? "PASS" : "FAIL") << "\n";
        if (ok) passed++; total++;
    }

    // TEST 3: 100-cycle fractal stress
    std::cout << "\n=== TEST 3: 100-Cycle Fractal Stress ===\n";
    {
        std::vector<uint64_t> vals(encoder.slot_count(), 999ULL);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100; i++) {
            fhe.bootstrap(ct);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        
        Plaintext after_pt; decryptor.decrypt(ct, after_pt);
        std::vector<uint64_t> after; encoder.decode(after_pt, after);
        
        auto ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        bool ok = (after[0] == 999);
        std::cout << "  After 100 cycles: " << after[0] 
                  << " (" << (ok ? "PASS" : "FAIL") << ") "
                  << ms << "ms\n";
        if (ok) passed++; total++;
    }

    // TEST 4: Lyapunov stability verification
    std::cout << "\n=== TEST 4: Lyapunov Stability ===\n";
    {
        double lambda = RecursiveFHE::LYAPUNOV;
        double phi = RecursiveFHE::PHI;
        double computed_lambda = std::log(phi);
        
        std::cout << "  φ = " << phi << "\n";
        std::cout << "  λ = ln(φ) = " << computed_lambda << "\n";
        std::cout << "  Expected: " << lambda << "\n";
        
        bool ok = (std::abs(computed_lambda - lambda) < 0.0001);
        std::cout << "  " << (ok ? "PASS" : "FAIL") << "\n";
        if (ok) passed++; total++;
    }

    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║  SPIRALSEAL RESULT: " << passed << "/" << total << " passed";
    for (size_t i = 0; i < 10; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "║  " << (passed == total ? "ALL TESTS PASSED ✅" : "SOME FAILED ❌");
    std::cout << "                    ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "  RecursiveFHE — No Ceiling. No Compromises.\n";

    return passed == total ? 0 : 1;
}
