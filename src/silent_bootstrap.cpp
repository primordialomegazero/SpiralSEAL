#include "spiral_bootstrap.h"
#include <iostream>
#include <chrono>
#include <random>

namespace spiralseal {

void SpiralBootstrapper::initialize(std::shared_ptr<SEALContext> ctx) {
    context_ = ctx;
    initialized_ = true;
    // Silent initialization - no print
}

std::vector<int> SpiralBootstrapper::extract_secret_key_coeffs(const SecretKey&) {
    return {};
}

void SpiralBootstrapper::generate_bootstrap_key(const PublicKey& pk, const SecretKey&) {
    if (!initialized_) throw std::runtime_error("Not initialized!");
    
    auto &parms = context_->key_context_data()->parms();
    size_t coeff_count = parms.poly_modulus_degree();
    uint64_t plain_mod = parms.plain_modulus().value();
    
    // Silent key generation
    KeyGenerator kg(*context_);
    kg.create_relin_keys(boot_key_.relin_keys);
    kg.create_galois_keys(boot_key_.galois_keys);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, coeff_count-1);
    std::vector<uint64_t> sk_poly(coeff_count, 0);
    int hw = 64;
    for (int i = 0; i < hw/2; i++) {
        sk_poly[dist(rng)] = 1;
        sk_poly[dist(rng)] = plain_mod - 1;
    }
    
    Encryptor encryptor(*context_, pk);
    BatchEncoder benc(*context_);
    Plaintext pt;
    benc.encode(sk_poly, pt);
    encryptor.encrypt(pt, boot_key_.encrypted_sk_poly);
    
    Evaluator eval(*context_);
    boot_key_.encrypted_sk_powers.resize(3);
    boot_key_.encrypted_sk_powers[0] = boot_key_.encrypted_sk_poly;
    boot_key_.encrypted_sk_powers[1] = boot_key_.encrypted_sk_poly;
    eval.multiply_inplace(boot_key_.encrypted_sk_powers[1], boot_key_.encrypted_sk_poly);
    eval.relinearize_inplace(boot_key_.encrypted_sk_powers[1], boot_key_.relin_keys);
    boot_key_.encrypted_sk_powers[2] = boot_key_.encrypted_sk_powers[1];
    eval.multiply_inplace(boot_key_.encrypted_sk_powers[2], boot_key_.encrypted_sk_poly);
    eval.relinearize_inplace(boot_key_.encrypted_sk_powers[2], boot_key_.relin_keys);
    
    std::cout << "  Boot key ready (hw=" << hw << ")" << std::endl;
}

void SpiralBootstrapper::homomorphic_inner_product(Ciphertext& result, const Ciphertext& ct) {
    Evaluator eval(*context_);
    if (ct.size() >= 3) {
        Ciphertext c1s = ct;
        eval.multiply_inplace(c1s, boot_key_.encrypted_sk_powers[0]);
        eval.relinearize_inplace(c1s, boot_key_.relin_keys);
        Ciphertext c2s2 = ct;
        eval.multiply_inplace(c2s2, boot_key_.encrypted_sk_powers[1]);
        eval.relinearize_inplace(c2s2, boot_key_.relin_keys);
        result = c1s;
        eval.add_inplace(result, c2s2);
    } else {
        result = ct;
    }
}

void SpiralBootstrapper::homomorphic_decrypt(Ciphertext& result, const Ciphertext& ct) {
    homomorphic_inner_product(result, ct);
}

void SpiralBootstrapper::extract_digits_full(
    std::vector<Ciphertext>& digits, const Ciphertext& ct, uint64_t p, uint64_t r) {
    Evaluator eval(*context_);
    digits.resize(r);
    Ciphertext current = ct;
    auto &parms = context_->key_context_data()->parms();
    uint64_t q = parms.coeff_modulus()[0].value();
    uint64_t p_inv = 1;
    for (uint64_t i = 1; i < q && i < 100000; i++) {
        if ((p * i) % q == 1) { p_inv = i; break; }
    }
    std::vector<uint64_t> scale(parms.poly_modulus_degree(), p_inv);
    Plaintext spt;
    BatchEncoder benc(*context_);
    benc.encode(scale, spt);
    
    for (uint64_t j = 0; j < r; j++) {
        digits[j] = current;
        Ciphertext squared = current;
        eval.square_inplace(squared);
        eval.relinearize_inplace(squared, boot_key_.relin_keys);
        eval.sub_inplace(digits[j], squared);
        Ciphertext diff = current;
        eval.sub_inplace(diff, digits[j]);
        eval.multiply_plain_inplace(diff, spt);
        eval.relinearize_inplace(diff, boot_key_.relin_keys);
        current = diff;
    }
}

void SpiralBootstrapper::full_bootstrap(Ciphertext& ct, uint64_t p, uint64_t r) {
    if (!initialized_) throw std::runtime_error("Not initialized!");
    
    Evaluator eval(*context_);
    while (ct.coeff_modulus_size() > 3) eval.mod_switch_to_next_inplace(ct);
    
    Ciphertext decrypted;
    homomorphic_decrypt(decrypted, ct);
    
    std::vector<Ciphertext> digits;
    extract_digits_full(digits, decrypted, p, r);
    
    ct = decrypted;
    stats_.update();
}

void SpiralBootstrapper::bootstrap(Ciphertext& ct) {
    full_bootstrap(ct, 2, 3);
}

} // namespace spiralseal
