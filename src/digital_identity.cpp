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

const std::string& DigitalIdentity::GetPrivateKey() const {
    return privateKey_;
}

const std::string& DigitalIdentity::GetPublicKey() const {
    return publicKey_;
}

