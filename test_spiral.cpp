#include <seal/seal.h>
#include "spiral_bootstrap.h"
#include <iostream>
using namespace seal;
using namespace spiralseal;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  SPIRALSEAL PHASE 1 TEST                       ║" << endl;
    cout << "║  Homomorphic Bootstrapping Core                ║" << endl;
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
    
    vector<uint64_t> v(benc.slot_count(), 42);
    Plaintext pt; benc.encode(v, pt);
    Ciphertext ct; enc.encrypt(pt, ct);
    
    cout << "\n  Initial noise: " << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    // Homomorphic bootstrap!
    bootstrapper.bootstrap(ct);
    
    cout << "  After bootstrap: " << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    Plaintext pt2; vector<uint64_t> res;
    dec.decrypt(ct, pt2); benc.decode(pt2, res);
    cout << "  Value: " << res[0] << " (expect 42)" << endl;
    
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    return 0;
}
