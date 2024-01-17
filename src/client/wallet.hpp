#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cryptopp/sha.h>
#include <cryptopp/ecp.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/randpool.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/base64.h>

class Wallet {
public:
    std::vector<std::string> wordlist_;
    std::vector<std::string> selectedWords_;
    CryptoPP::SecByteBlock seed_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey ecPrivateKey_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey ecPublicKey_;
    std::string privateKey_;
    std::string publicKey_;
    std::map<std::time_t, double> balance_;


public:
    Wallet(const std::string &wordlistFileName) {
        LoadWordList(wordlistFileName);
    }

    bool LoadWordList(const std::string &filename) {
        std::ifstream file(filename);

        if (!file.good())
        {
            std::cerr << "Can't open world list file" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::string word;
        while (std::getline(file, word))
        {
            wordlist_.push_back(word);
        }

        return true;
    }

    bool SelectRandomWords(size_t count) {
        if (count > wordlist_.size())
        {
            return false;
        }

        CryptoPP::AutoSeededRandomPool rng;
        selectedWords_.resize(count);

        for (size_t i = 0; i < count; ++i)
        {
            size_t randomIndex = rng.GenerateWord32(0, wordlist_.size() - 1);
            selectedWords_[i] = wordlist_[randomIndex];
        }

        return true;
    }

    bool DeriveSeedFromWords() {
        std::string joinedWords;
        for (const std::string &word : selectedWords_)
        {
            joinedWords += word;
        }

        CryptoPP::SHA256 sha256;
        seed_.resize(sha256.DigestSize());
        sha256.CalculateDigest(seed_, reinterpret_cast<const CryptoPP::byte *>(joinedWords.data()), joinedWords.size());

        return true;
    }

    bool GenerateECDSAKeyPair() {
        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256> ecdsa;

        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
        privateKey.Initialize(rng, CryptoPP::ASN1::secp256r1());

        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
        privateKey.MakePublicKey(publicKey);

        CryptoPP::ByteQueue queue;
        privateKey.Save(queue);
        privateKey_.resize(queue.CurrentSize());
        queue.Get(reinterpret_cast<CryptoPP::byte *>(&privateKey_[0]), privateKey_.size());

        queue.Clear();
        publicKey.Save(queue);
        publicKey_.resize(queue.CurrentSize());
        queue.Get(reinterpret_cast<CryptoPP::byte *>(&publicKey_[0]), publicKey_.size());

        return true;
    }

    const std::string &GetPrivateKey() const {
        return privateKey_;
    }

    const std::string &GetPublicKey() const {
        return publicKey_;
    }

    const std::vector<std::string> &GetSelectedWords() const
    {
        return selectedWords_;
    }
};