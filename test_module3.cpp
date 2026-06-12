#include <seal/seal.h>
#include "spiral_bootstrap.h"
#include <iostream>
using namespace seal;
using namespace spiralseal;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  SPIRALSEAL MODULE 3 TEST                     ║" << endl;
    cout << "║  Digit Extraction (Chen & Han)                ║" << endl;
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
    
    Encryptor enc(*ctx, pk);
    Decryptor dec(*ctx, sk);
    BatchEncoder benc(*ctx);
    
    // Initialize SpiralSEAL
    SpiralBootstrapper bootstrapper;
    bootstrapper.initialize(ctx);
    bootstrapper.generate_bootstrap_key(pk, sk);
    
    cout << "\n═══ DIGIT EXTRACTION TEST ═══" << endl;
    
    // Encrypt a value
    vector<uint64_t> v(benc.slot_count(), 42);
    Plaintext pt; benc.encode(v, pt);
    Ciphertext ct; enc.encrypt(pt, ct);
    
    cout << "Original value: 42" << endl;
    cout << "Noise budget: " << dec.invariant_noise_budget(ct) << " bits" << endl;
    
    // Extract digits in base 2
    cout << "\nExtracting digits (base 2, r=3):" << endl;
    vector<Ciphertext> digits;
    bootstrapper.extract_digits_full(digits, ct, 2, 3);
    
    cout << "\nDigits extracted: " << digits.size() << endl;
    
    // Verify
    for (size_t i = 0; i < digits.size(); i++) {
        Plaintext pc; vector<uint64_t> r;
        dec.decrypt(digits[i], pc); benc.decode(pc, r);
        cout << "  Digit[" << i << "] = " << r[0] << endl;
    }
    
    cout << "\n╔══════════════════════════════════════════════╗" << endl;
    cout << "║  ΦΩ0 — I AM THAT I AM                       ║" << endl;
    cout << "╚══════════════════════════════════════════════╝" << endl;
    
    return 0;
}
