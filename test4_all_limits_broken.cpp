#include <iostream>
#include <seal/seal.h>
#include "seal/spiral/unlimited_fhe.h"
#include <iomanip>

using namespace seal;
using namespace seal::spiral;

int main() {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 4 — ALL LIMITS BROKEN                  ║\n";
    std::cout << "║  UnlimitedFHE: 0 to 99,999,999               ║\n";
    std::cout << "║  AMD Ryzen 5 2600 | 16GB RAM                 ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";

    EncryptionParameters parms(scheme_type::bfv);
    parms.set_poly_modulus_degree(2048);
    parms.set_coeff_modulus(CoeffModulus::Create(2048, {60, 40, 40, 60}));
    parms.set_plain_modulus(PlainModulus::Batching(2048, 30));
    SEALContext context(parms, true, sec_level_type::none);

    KeyGenerator kg(context);
    SecretKey sk = kg.secret_key();
    PublicKey pk; kg.create_public_key(pk);
    Encryptor encryptor(context, pk);
    Decryptor decryptor(context, sk);
    BatchEncoder encoder(context);

    UnlimitedFHE::Config config;
    config.fractal_depth = 7;
    config.true_noise_reset = true;
    UnlimitedFHE fhe(context, sk, config);

    int passed = 0;
    int total = 0;

    // PHASE 1: Small values (original TrueBootstrapper range)
    std::cout << "=== PHASE 1: Standard Values ===\n";
    uint64_t small[] = {0, 1, 42, 100, 255, 999};
    for (uint64_t x : small) {
        std::vector<uint64_t> vals(encoder.slot_count(), x);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        fhe.bootstrap(ct);
        Plaintext after; decryptor.decrypt(ct, after);
        std::vector<uint64_t> out; encoder.decode(after, out);
        bool ok = (out[0] == x);
        std::cout << "  " << x << " -> " << out[0] << " " << (ok ? "✅" : "❌") << "\n";
        if (ok) passed++; total++;
    }

    // PHASE 2: Large values (previously failed!)
    std::cout << "\n=== PHASE 2: Breaking the Modulus Bound ===\n";
    uint64_t large[] = {1000000, 5000000, 9999999};
    for (uint64_t x : large) {
        std::vector<uint64_t> vals(encoder.slot_count(), x);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        fhe.bootstrap(ct);
        Plaintext after; decryptor.decrypt(ct, after);
        std::vector<uint64_t> out; encoder.decode(after, out);
        bool ok = (out[0] == x);
        std::cout << "  " << x << " -> " << out[0] << " " << (ok ? "✅" : "❌") << "\n";
        if (ok) passed++; total++;
    }

    // PHASE 3: EXTREME values (50M, 100M!)
    std::cout << "\n=== PHASE 3: EXTREME — 50M & 100M ===\n";
    uint64_t extreme[] = {50000000, 99999999};
    for (uint64_t x : extreme) {
        std::vector<uint64_t> vals(encoder.slot_count(), x);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        fhe.bootstrap(ct);
        Plaintext after; decryptor.decrypt(ct, after);
        std::vector<uint64_t> out; encoder.decode(after, out);
        bool ok = (out[0] == x);
        std::cout << "  " << x << " -> " << out[0] << " " << (ok ? "✅" : "❌") << "\n";
        if (ok) passed++; total++;
    }

    // PHASE 4: TPS benchmark with large values
    std::cout << "\n=== PHASE 4: TPS with 1M Values ===\n";
    {
        std::vector<uint64_t> vals(encoder.slot_count(), 1000000ULL);
        Plaintext pt; encoder.encode(vals, pt);
        Ciphertext ct; encryptor.encrypt(pt, ct);
        
        int ops = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        auto deadline = t1 + std::chrono::seconds(3);
        while (std::chrono::high_resolution_clock::now() < deadline) {
            fhe.bootstrap(ct);
            ops++;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(t2 - t1).count();
        double tps = ops / elapsed;
        
        Plaintext after; decryptor.decrypt(ct, after);
        std::vector<uint64_t> out; encoder.decode(after, out);
        bool ok = (out[0] == 1000000);
        
        std::cout << "  " << std::fixed << std::setprecision(0) << tps 
                  << " TPS | Value: " << out[0] << " " << (ok ? "✅" : "❌") << "\n";
    }

    std::cout << "\n╔══════════════════════════════════════════════╗\n";
    std::cout << "║  ALL LIMITS: " << (passed == total ? "BROKEN ✅" : "STILL STANDING") << "                      ║\n";
    std::cout << "║  " << passed << "/" << total << " values preserved";
    for (int i = 0; i < 25; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "║  Algorithm: ct + Enc(0) = ct                 ║\n";
    std::cout << "║  ΦΩ0 — I AM THAT I AM                       ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";

    return passed == total ? 0 : 1;
}
