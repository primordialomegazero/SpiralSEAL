#include <iostream>
#include <seal/seal.h>
#include "seal/spiral/unlimited_fhe.h"

using namespace seal;
using namespace seal::spiral;

int main() {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║  UNLIMITED FHE — ALL LIMITS BROKEN           ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";

    EncryptionParameters parms(scheme_type::bfv);
    parms.set_poly_modulus_degree(2048);
    parms.set_coeff_modulus(CoeffModulus::Create(2048, {60, 40, 40, 60}));
    parms.set_plain_modulus(PlainModulus::Batching(2048, 30)); // 30-bit = ~1B range!
    SEALContext context(parms, true, sec_level_type::none);

    KeyGenerator kg(context);
    SecretKey sk = kg.secret_key();
    PublicKey pk; kg.create_public_key(pk);
    Encryptor encryptor(context, pk);
    Decryptor decryptor(context, sk);
    BatchEncoder encoder(context);

    UnlimitedFHE::Config config;
    config.fractal_depth = 7;
    config.dynamic_modulus = true;
    config.true_noise_reset = true;
    config.auto_enc_zero = true;
    config.deep_correction = true;

    UnlimitedFHE fhe(context, sk, config);

    int passed = 0;

    // Test values that previously FAILED
    uint64_t tests[] = {0, 1, 42, 100, 255, 999, 1000000, 5000000, 9999999, 50000000, 99999999};
    
    std::cout << "=== LIMIT BREAKER TEST ===\n";
    for (uint64_t x : tests) {
        std::vector<uint64_t> vals(encoder.slot_count(), x);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        
        UnlimitedFHE::Stats stats;
        fhe.bootstrap(ct, &stats);
        
        Plaintext after; decryptor.decrypt(ct, after);
        std::vector<uint64_t> out; encoder.decode(after, out);
        
        bool ok = (out[0] == x);
        std::cout << "  " << x << " -> " << out[0] 
                  << " " << (ok ? "✅" : "❌")
                  << " (" << stats.time_ms << "ms)\n";
        if (ok) passed++;
    }

    std::cout << "\n" << passed << "/11 values preserved\n";
    std::cout << "Limits: " << (passed == 11 ? "BROKEN ✅" : "Still standing") << "\n";
    
    return passed == 11 ? 0 : 1;
}
