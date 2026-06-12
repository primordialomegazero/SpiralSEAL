#include <seal/seal.h>
#include "spiral_bootstrap.h"
#include <iostream>
using namespace seal;
using namespace spiralseal;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  SPIRALSEAL PHASE 3 - FULL BOOTSTRAP TEST      ║" << endl;
    cout << "║  Frobenius + Linear Transforms                 ║" << endl;
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
    
    // Initialize SpiralSEAL Phase 3
    SpiralBootstrapper bootstrapper;
    bootstrapper.initialize(ctx);
    bootstrapper.generate_bootstrap_key(pk, sk);
    
    cout << "\n═══ FULL BOOTSTRAP TEST ═══" << endl;
    
    // Encrypt 42
    vector<uint64_t> v(benc.slot_count(), 42);
    Plaintext pt; benc.encode(v, pt);
    Ciphertext ct; enc.encrypt(pt, ct);
    
    cout << "Initial: value=42, noise=" << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    // Add noise via multiply
    vector<uint64_t> v3(benc.slot_count(), 3);
    Plaintext pt3; benc.encode(v3, pt3);
    Ciphertext ct3; enc.encrypt(pt3, ct3);
    
    eval.multiply_inplace(ct, ct3);
    eval.relinearize_inplace(ct, rlk);
    eval.mod_switch_to_next_inplace(ct);
    
    cout << "After multiply: noise=" << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    // FULL BOOTSTRAP!
    bootstrapper.full_bootstrap(ct, 2, 3);
    
    cout << "\nAfter full bootstrap: noise=" << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    Plaintext pc; vector<uint64_t> r;
    dec.decrypt(ct, pc); benc.decode(pc, r);
    cout << "Value: " << r[0] << " (started at 42)" << endl;
    
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    return 0;
}
