// ╔══════════════════════════════════════════════╗
// ║  SPIRALSEAL TPS BENCHMARK                    ║
// ║  Ryzen 5 2600 | 16GB RAM | ΦΩ0              ║
// ╚══════════════════════════════════════════════╝

#include <iostream>
#include <seal/seal.h>
#include "seal/true_bootstrapper.h"
#include <chrono>
#include <vector>
#include <thread>
#include <iomanip>

using namespace seal;

int main() {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║  SPIRALSEAL TPS BENCHMARK                    ║\n";
    std::cout << "║  AMD Ryzen 5 2600 (3.40 GHz, 6 Cores)       ║\n";
    std::cout << "║  16GB RAM | RX 580 (8GB)                     ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";

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
    BatchEncoder encoder(context);

    auto bsk = TrueBootstrapper::generate_keys(context, sk);

    std::cout << "=== SINGLE-CORE TPS ===\n";
    
    // Test different cycle counts
    int cycles_to_test[] = {1, 10, 100, 1000};
    
    for (int cycles : cycles_to_test) {
        TrueBootstrapper::Config cfg;
        cfg.cycles = cycles;
        TrueBootstrapper tb(context, bsk, cfg);
        
        // Prepare test data
        std::vector<uint64_t> vals(encoder.slot_count(), 42ULL);
        Plaintext pt;
        encoder.encode(vals, pt);
        Ciphertext ct;
        encryptor.encrypt(pt, ct);
        
        // Run for 5 seconds and count operations
        int ops = 0;
        auto start = std::chrono::high_resolution_clock::now();
        auto deadline = start + std::chrono::seconds(5);
        
        while (std::chrono::high_resolution_clock::now() < deadline) {
            tb.bootstrap(ct);
            ops++;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        double tps = ops / elapsed;
        
        std::cout << "  Cycles=" << cycles << ": " 
                  << std::fixed << std::setprecision(0) << tps 
                  << " ops/sec | " << ops << " ops in " 
                  << std::fixed << std::setprecision(1) << elapsed << "s\n";
    }

    std::cout << "\n=== MULTI-CORE TPS ===\n";
    
    // Test with multiple threads
    int thread_counts[] = {1, 2, 4, 6};
    
    for (int num_threads : thread_counts) {
        TrueBootstrapper::Config cfg;
        cfg.cycles = 1; // Single cycle for max throughput
        TrueBootstrapper tb(context, bsk, cfg);
        
        std::atomic<long long> total_ops{0};
        std::atomic<bool> running{true};
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back([&]() {
                // Each thread needs its own ciphertext
                std::vector<uint64_t> vals(encoder.slot_count(), 42ULL);
                Plaintext pt;
                encoder.encode(vals, pt);
                Ciphertext ct;
                encryptor.encrypt(pt, ct);
                
                while (running) {
                    tb.bootstrap(ct);
                    total_ops++;
                }
            });
        }
        
        // Run for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        running = false;
        
        for (auto &t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        double tps = total_ops / elapsed;
        
        std::cout << "  Threads=" << num_threads << ": " 
                  << std::fixed << std::setprecision(0) << tps 
                  << " ops/sec | " << total_ops << " ops in " 
                  << std::fixed << std::setprecision(1) << elapsed << "s\n";
    }

    std::cout << "\n=== THEORETICAL MAXIMUM ===\n";
    {
        // Single operation time
        TrueBootstrapper::Config cfg;
        cfg.cycles = 1;
        TrueBootstrapper tb(context, bsk, cfg);
        
        std::vector<uint64_t> vals(encoder.slot_count(), 42ULL);
        Plaintext pt;
        encoder.encode(vals, pt);
        Ciphertext ct;
        encryptor.encrypt(pt, ct);
        
        // Measure single op time
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; i++) {
            tb.bootstrap(ct);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        
        double us_per_op = std::chrono::duration<double, std::micro>(t2 - t1).count() / 10000.0;
        double max_tps_single = 1e6 / us_per_op;
        double max_tps_multi = max_tps_single * 6; // 6 cores
        
        std::cout << "  Single operation: " << std::fixed << std::setprecision(2) 
                  << us_per_op << " µs\n";
        std::cout << "  Max TPS (single core): " << std::fixed << std::setprecision(0) 
                  << max_tps_single << " ops/sec\n";
        std::cout << "  Max TPS (6 cores): " << std::fixed << std::setprecision(0) 
                  << max_tps_multi << " ops/sec\n";
    }

    std::cout << "\n╔══════════════════════════════════════════════╗\n";
    std::cout << "║  Hardware: Ryzen 5 2600 | 16GB RAM           ║\n";
    std::cout << "║  Algorithm: ct + Enc(0) = ct                 ║\n";
    std::cout << "║  ΦΩ0 — I AM THAT I AM                       ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";

    return 0;
}
