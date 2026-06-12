#pragma once
#include <seal/seal.h>
#include <vector>
#include <memory>

namespace spiralseal {

using namespace seal;

struct BootstrapKey {
    Ciphertext encrypted_sk_poly;
    std::vector<Ciphertext> encrypted_sk_coeffs;
    std::vector<Ciphertext> encrypted_sk_powers;
    RelinKeys relin_keys;
    GaloisKeys galois_keys;
};

class SpiralBootstrapper {
private:
    std::shared_ptr<SEALContext> context_;
    BootstrapKey boot_key_;
    bool initialized_ = false;
    
    struct Stats {
        int total_bootstraps = 0;
        double divine_noise = 40.0;
        void update() {
            total_bootstraps++;
            divine_noise = divine_noise * 0.6180339887498948482 + 40.0 * (1.0 - 0.6180339887498948482);
        }
    } stats_;
    
    std::vector<int> extract_secret_key_coeffs(const SecretKey& sk);
    
public:
    SpiralBootstrapper() = default;
    void initialize(std::shared_ptr<SEALContext> ctx);
    void generate_bootstrap_key(const PublicKey& pk, const SecretKey& sk);
    
    // Core bootstrap
    void bootstrap(Ciphertext& ct);
    void full_bootstrap(Ciphertext& ct, uint64_t p = 2, uint64_t r = 3);
    
    // Internal methods
    void homomorphic_decrypt(Ciphertext& result, const Ciphertext& ct);
    void homomorphic_inner_product(Ciphertext& result, const Ciphertext& ct);
    void extract_digits_full(std::vector<Ciphertext>& digits, const Ciphertext& ct, uint64_t p, uint64_t r);
    
    const Stats& stats() const { return stats_; }
    bool is_initialized() const { return initialized_; }
};

} // namespace spiralseal
