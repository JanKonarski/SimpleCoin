#include "digital_identity.hpp"
#include <fstream>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/ec.h>

DigitalIdentity::DigitalIdentity(const std::string& wordlistFileName) {
    LoadWordList(wordlistFileName);
}

DigitalIdentity::~DigitalIdentity() {
    if (ecKeyPair_) {
        EC_KEY_free(ecKeyPair_);
    }
}

bool DigitalIdentity::LoadWordList(const std::string& filename) {
    std::ifstream file(filename);

    if (!file) {
        return false;
    }

    std::string word;
    while (std::getline(file, word)) {
        wordlist_.push_back(word);
    }

    return true;
}

bool DigitalIdentity::SelectRandomWords(size_t count) {
    if (count > wordlist_.size()) {
        return false;
    }

    while (selectedWords_.size() < count) {
        size_t randomIndex = rand() % wordlist_.size();
        selectedWords_.push_back(wordlist_[randomIndex]);
    }

    return true;
}

bool DigitalIdentity::DeriveSeedFromWords() {
    std::string joinedWords;
    for (const std::string& word : selectedWords_) {
        joinedWords += word;
    }

    seed_.resize(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const uint8_t*>(joinedWords.c_str()), joinedWords.size(), seed_.data());

    return true;
}

bool DigitalIdentity::GenerateECDSAKeyPair() {
    // Create an EC_KEY structure and set it to use the secp256k1 curve (Bitcoin curve)
    ecKeyPair_ = EC_KEY_new_by_curve_name(NID_secp256k1);

    // Allocate memory for a BIGNUM to store the private key
    BIGNUM* privateKey = BN_new();

    // Convert the seed data into a BIGNUM private key
    BN_bin2bn(seed_.data(), static_cast<int>(seed_.size()), privateKey);

    // Set the private key in the EC_KEY structure
    EC_KEY_set_private_key(ecKeyPair_, privateKey);

    // Create an EC_POINT structure to store the public key
    EC_POINT* publicKeyPoint = EC_POINT_new(EC_KEY_get0_group(ecKeyPair_));

    // Compute the public key point by multiplying the private key with the base point on the curve
    EC_POINT_mul(EC_KEY_get0_group(ecKeyPair_), publicKeyPoint, privateKey, NULL, NULL, NULL);

    // Set the public key in the EC_KEY structure
    EC_KEY_set_public_key(ecKeyPair_, publicKeyPoint);

    // Get a const pointer to the private key BIGNUM
    const BIGNUM* privKeyBN = EC_KEY_get0_private_key(ecKeyPair_);

    // Get a const pointer to the public key EC_POINT
    const EC_POINT* pubKeyPoint = EC_KEY_get0_public_key(ecKeyPair_);

    // Convert the private key BIGNUM to a hexadecimal string
    char* privKeyHex = BN_bn2hex(privKeyBN);

    // Convert the public key EC_POINT to a hexadecimal string (uncompressed format)
    char* pubKeyHex = EC_POINT_point2hex(EC_KEY_get0_group(ecKeyPair_), pubKeyPoint, POINT_CONVERSION_UNCOMPRESSED, NULL);

    // Store the private and public keys as strings in the class
    privateKey_ = privKeyHex;
    publicKey_ = pubKeyHex;

    // Free the memory allocated for the temporary hexadecimal strings
    OPENSSL_free(privKeyHex);
    OPENSSL_free(pubKeyHex);

    // Return true to indicate successful key pair generation
    return true;
}


const std::string& DigitalIdentity::GetPrivateKey() const {
    return privateKey_;
}

const std::string& DigitalIdentity::GetPublicKey() const {
    return publicKey_;
}

