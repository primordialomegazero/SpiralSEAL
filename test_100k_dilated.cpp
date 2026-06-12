#include <seal/seal.h>
#include "spiral_bootstrap.h"
#include "phi_time_dilation.h"
#include <iostream>
#include <iomanip>
using namespace seal;
using namespace spiralseal;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  Φ-100K TIME-DILATED BOOTSTRAP TEST            ║" << endl;
    cout << "║  100,000 Multiplies with Divine Timing         ║" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    EncryptionParameters parms(scheme_type::bfv);
    size_t n = 8192;
    parms.set_poly_modulus_degree(n);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(n));
    parms.set_plain_modulus(PlainModulus::Batching(n, 20));
    auto ctx = make_shared<SEALContext>(parms, true, sec_level_type::tc128);
    
    KeyGenerator kg(*ctx);
    SecretKey sk = kg.secret_key();
    PublicKey pk; kg.create_public_key(pk);
    RelinKeys rlk; kg.create_relin_keys(rlk);
    
    Encryptor enc(*ctx, pk);
    Decryptor dec(*ctx, sk);
    Evaluator eval(*ctx);
    BatchEncoder benc(*ctx);
    
    SpiralBootstrapper bootstrapper;
    bootstrapper.initialize(ctx);
    bootstrapper.generate_bootstrap_key(pk, sk);
    
    PhiTimeDilation timer;
    
    cout << "\n═══ 100K TIME-DILATED MULTIPLY ═══" << endl;
    cout << "  Starting value: 7" << endl;
    cout << "  Target: 100,000 iterations" << endl;
    cout << "  Strategy: φ-weighted divine timing" << endl;
    
    vector<uint64_t> v7(benc.slot_count(), 7);
    vector<uint64_t> v3(benc.slot_count(), 3);
    Plaintext pt7, pt3;
    benc.encode(v7, pt7);
    benc.encode(v3, pt3);
    
    Ciphertext ct, ct3;
    enc.encrypt(pt7, ct);
    
    int ops_since_bootstrap = 0;
    int survived = 0;
    int bootstraps_done = 0;
    int correct = 0;
    
    auto global_start = high_resolution_clock::now();
    
    for (int i = 1; i <= 100000; i++) {
        // Time this operation
        auto op_start = high_resolution_clock::now();
        
        // Fresh ct3
        enc.encrypt(pt3, ct3);
        eval.mod_switch_to_inplace(ct3, ct.parms_id());
        eval.multiply_inplace(ct, ct3);
        eval.relinearize_inplace(ct, rlk);
        if (ct.coeff_modulus_size() > 1) eval.mod_switch_to_next_inplace(ct);
        
        auto op_end = high_resolution_clock::now();
        double op_ms = duration_cast<microseconds>(op_end - op_start).count() / 1000.0;
        timer.record_operation(op_ms);
        
        ops_since_bootstrap++;
        survived++;
        
        // Time-dilated bootstrap
        if (timer.should_bootstrap(ops_since_bootstrap) || ct.coeff_modulus_size() < 2) {
            auto bs_start = high_resolution_clock::now();
            
            try {
                bootstrapper.full_bootstrap(ct);
                bootstraps_done++;
                ops_since_bootstrap = 0;
            } catch (...) {
                cout << "  Bootstrap failed at iter " << i << endl;
            }
            
            auto bs_end = high_resolution_clock::now();
            double bs_ms = duration_cast<milliseconds>(bs_end - bs_start).count();
            timer.record_bootstrap(bs_ms);
        }
        
        // Checkpoint every 10K
        if (i % 10000 == 0) {
            Plaintext pc; vector<uint64_t> r;
            dec.decrypt(ct, pc); benc.decode(pc, r);
            
            uint64_t expected = 7;
            for (int j = 0; j < i; j++) expected = (expected * 3) % 1032193;
            
            cout << "  iter " << setw(5) << i 
                 << ": val=" << setw(7) << r[0]
                 << " " << (r[0] == expected ? "✓" : "✗")
                 << " noise=" << dec.invariant_noise_budget(ct)
                 << " bs=" << bootstraps_done
                 << " interval=" << fixed << setprecision(1) << timer.get_divine_interval()
                 << " dilation=" << setprecision(2) << timer.time_dilation_factor() << "x"
                 << endl;
            
            if (r[0] == expected) correct++;
        }
    }
    
    auto global_end = high_resolution_clock::now();
    auto total_sec = duration_cast<seconds>(global_end - global_start).count();
    
    cout << "\n═══ 100K RESULTS ═══" << endl;
    cout << "  Survived: " << survived << "/100000" << endl;
    cout << "  Bootstraps: " << bootstraps_done << endl;
    cout << "  Total time: " << total_sec << "s" << endl;
    cout << "  Rate: " << (survived / (total_sec + 0.001)) << " ops/sec" << endl;
    
    timer.print_stats();
    
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    return 0;
}
