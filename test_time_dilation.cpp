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
    cout << "║  Φ-TIME DILATION BOOTSTRAP TEST                ║" << endl;
    cout << "║  Recursive Fractal Timing Optimization         ║" << endl;
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
    
    // Initialize SpiralSEAL
    SpiralBootstrapper bootstrapper;
    bootstrapper.initialize(ctx);
    bootstrapper.generate_bootstrap_key(pk, sk);
    
    // Initialize Time Dilation
    PhiTimeDilation timer;
    
    cout << "\n═══ TIME-DILATED 100x MULTIPLY ═══" << endl;
    
    vector<uint64_t> v(benc.slot_count(), 7);
    vector<uint64_t> v3(benc.slot_count(), 3);
    Plaintext pt, pt3;
    benc.encode(v, pt);
    benc.encode(v3, pt3);
    
    Ciphertext ct, ct3;
    enc.encrypt(pt, ct);
    
    int ops_since_bootstrap = 0;
    int correct = 0;
    
    for (int i = 1; i <= 100; i++) {
        // Measure operation time
        auto op_start = high_resolution_clock::now();
        
        enc.encrypt(pt3, ct3);
        eval.mod_switch_to_inplace(ct3, ct.parms_id());
        eval.multiply_inplace(ct, ct3);
        eval.relinearize_inplace(ct, rlk);
        if (ct.coeff_modulus_size() > 1) eval.mod_switch_to_next_inplace(ct);
        
        auto op_end = high_resolution_clock::now();
        double op_time = duration_cast<microseconds>(op_end - op_start).count() / 1000.0;
        timer.record_operation(op_time);
        
        ops_since_bootstrap++;
        
        // Time-dilated bootstrap decision
        if (timer.should_bootstrap(ops_since_bootstrap) || ct.coeff_modulus_size() < 2) {
            auto bs_start = high_resolution_clock::now();
            
            bootstrapper.full_bootstrap(ct);
            
            auto bs_end = high_resolution_clock::now();
            double bs_time = duration_cast<milliseconds>(bs_end - bs_start).count();
            timer.record_bootstrap(bs_time);
            
            ops_since_bootstrap = 0;
        }
        
        // Check value
        Plaintext pc; vector<uint64_t> r;
        dec.decrypt(ct, pc); benc.decode(pc, r);
        
        uint64_t expected = 7;
        for (int j = 0; j < i; j++) expected = (expected * 3) % 1032193;
        
        if (r[0] == expected) correct++;
        
        if (i % 20 == 0 || i <= 5) {
            cout << "  iter " << setw(3) << i << ": val=" << r[0] 
                 << " " << (r[0] == expected ? "✓" : "✗")
                 << " interval=" << fixed << setprecision(1) << timer.get_divine_interval()
                 << " dilation=" << setprecision(3) << timer.time_dilation_factor() << "x"
                 << endl;
        }
    }
    
    cout << "\n  Correct: " << correct << "/100" << endl;
    timer.print_stats();
    
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    return 0;
}
