#include "PhoneVRNetwork.h"
#include <windows.h>
#include <bcrypt.h>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace phonevr {

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

SecureChannel::SecureChannel() {
    BCRYPT_ALG_HANDLE alg_handle = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &alg_handle, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (NT_SUCCESS(status)) {
        status = BCryptSetProperty(
            alg_handle, BCRYPT_CHAINING_MODE,
            (PUINT8)BCRYPT_CHAIN_MODE_GCM,
            (ULONG)(sizeof(BCRYPT_CHAIN_MODE_GCM) + 1), 0);
        if (NT_SUCCESS(status)) {
            aes_handle_ = alg_handle;
        } else {
            BCryptCloseAlgorithmProvider(alg_handle, 0);
        }
    }
}

SecureChannel::~SecureChannel() {
    if (aes_handle_) {
        BCryptCloseAlgorithmProvider(static_cast<BCRYPT_ALG_HANDLE>(aes_handle_), 0);
    }
    SecureZeroMemory(key_, KEY_SIZE);
}

std::vector<uint8_t> SecureChannel::initialize_session() {
    NTSTATUS status = BCryptGenRandom(
        nullptr, reinterpret_cast<PUINT8>(key_), KEY_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!NT_SUCCESS(status)) {
        throw std::runtime_error("Failed to generate session key");
    }
    has_key_ = true;
    return std::vector<uint8_t>(key_, key_ + KEY_SIZE);
}

void SecureChannel::set_session_key(const uint8_t* key, size_t key_size) {
    if (key_size != KEY_SIZE) {
        throw std::invalid_argument("Invalid key size");
    }
    std::memcpy(key_, key, KEY_SIZE);
    has_key_ = true;
}

std::vector<uint8_t> SecureChannel::encrypt(const uint8_t* data, size_t size) {
    if (!has_key_) throw std::runtime_error("No session key");
    if (!aes_handle_) throw std::runtime_error("AES not initialized");

    BCRYPT_ALG_HANDLE alg = static_cast<BCRYPT_ALG_HANDLE>(aes_handle_);

    // Get auth tag length
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT tag_lengths = {0};
    ULONG result_size = 0;
    BCryptGetProperty(alg, BCRYPT_AUTH_TAG_LENGTH,
                      reinterpret_cast<PUINT8>(&tag_lengths),
                      sizeof(tag_lengths), &result_size, 0);

    // Generate nonce
    uint8_t nonce[NONCE_SIZE];
    BCryptGenRandom(nullptr, nonce, NONCE_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    // Get key object size
    ULONG key_obj_size = 0;
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
                      reinterpret_cast<PUINT8>(&key_obj_size),
                      sizeof(key_obj_size), &result_size, 0);

    std::vector<uint8_t> key_obj(key_obj_size);
    BCRYPT_KEY_HANDLE key_handle = nullptr;

    NTSTATUS status = BCryptGenerateSymmetricKey(
        alg, &key_handle, key_obj.data(), key_obj_size,
        const_cast<PUINT8>(key_), KEY_SIZE, 0);
    if (!NT_SUCCESS(status)) throw std::runtime_error("Failed to generate key");

    // Encrypt
    std::vector<uint8_t> ciphertext(size + TAG_SIZE);
    ULONG ciphertext_size = 0;

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_INIT_AUTH_MODE_INFO(auth_info);
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = NONCE_SIZE;
    auth_info.pbTag = ciphertext.data() + size;
    auth_info.cbTag = TAG_SIZE;
    auth_info.pbAuthData = nullptr;
    auth_info.cbAuthData = 0;

    status = BCryptEncrypt(key_handle,
                           const_cast<PUINT8>(data),
                           static_cast<ULONG>(size),
                           &auth_info,
                           nullptr, 0,
                           ciphertext.data(),
                           static_cast<ULONG>(ciphertext.size()),
                           &ciphertext_size,
                           0);

    BCryptDestroyKey(key_handle);

    if (!NT_SUCCESS(status)) throw std::runtime_error("Encryption failed");

    // Build result: nonce + ciphertext + tag
    std::vector<uint8_t> result(NONCE_SIZE + ciphertext_size);
    std::memcpy(result.data(), nonce, NONCE_SIZE);
    std::memcpy(result.data() + NONCE_SIZE, ciphertext.data(), ciphertext_size);

    return result;
}

std::vector<uint8_t> SecureChannel::decrypt(const uint8_t* data, size_t size) {
    if (!has_key_) throw std::runtime_error("No session key");
    if (!aes_handle_) throw std::runtime_error("AES not initialized");
    if (size < NONCE_SIZE + TAG_SIZE) throw std::runtime_error("Invalid packet size");

    BCRYPT_ALG_HANDLE alg = static_cast<BCRYPT_ALG_HANDLE>(aes_handle_);

    const uint8_t* nonce = data;
    const uint8_t* ciphertext = data + NONCE_SIZE;
    size_t ciphertext_size = size - NONCE_SIZE - TAG_SIZE;
    const uint8_t* tag = data + size - TAG_SIZE;

    ULONG key_obj_size = 0;
    ULONG result_size = 0;
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
                      reinterpret_cast<PUINT8>(&key_obj_size),
                      sizeof(key_obj_size), &result_size, 0);

    std::vector<uint8_t> key_obj(key_obj_size);
    BCRYPT_KEY_HANDLE key_handle = nullptr;

    NTSTATUS status = BCryptGenerateSymmetricKey(
        alg, &key_handle, key_obj.data(), key_obj_size,
        const_cast<PUINT8>(key_), KEY_SIZE, 0);
    if (!NT_SUCCESS(status)) throw std::runtime_error("Failed to generate key");

    std::vector<uint8_t> plaintext(ciphertext_size);
    ULONG plaintext_size = 0;

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_INIT_AUTH_MODE_INFO(auth_info);
    auth_info.pbNonce = const_cast<PUINT8>(nonce);
    auth_info.cbNonce = NONCE_SIZE;
    auth_info.pbTag = const_cast<PUINT8>(tag);
    auth_info.cbTag = TAG_SIZE;
    auth_info.pbAuthData = nullptr;
    auth_info.cbAuthData = 0;

    status = BCryptDecrypt(key_handle,
                           const_cast<PUINT8>(ciphertext),
                           static_cast<ULONG>(ciphertext_size),
                           &auth_info,
                           nullptr, 0,
                           plaintext.data(),
                           static_cast<ULONG>(plaintext.size()),
                           &plaintext_size,
                           0);

    BCryptDestroyKey(key_handle);

    if (!NT_SUCCESS(status)) throw std::runtime_error("Decryption failed - auth tag mismatch");

    plaintext.resize(plaintext_size);
    return plaintext;
}

} // namespace phonevr
