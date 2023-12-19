#ifndef WALLET_HPP
#define WALLET_HPP

#include <string>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/ecp.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>
#include "transaction.hpp"
#include <iostream>

class Wallet
{
public:
    Wallet(const std::string &wordlistFileName);
    ~Wallet();

    bool LoadWordList(const std::string &filename);
    bool SelectRandomWords(size_t count);
    bool DeriveSeedFromWords();
    bool GenerateECDSAKeyPair();
    Transaction sendToken(const Transaction::Input &input, const std::vector<Transaction::Output> &outputs);
    double CalculateBalance(const std::string &pubkey, const std::vector<std::vector<Transaction>> &transactions);

    const std::string &GetPrivateKey() const;
    const std::string &GetPublicKey() const;
    const std::vector<std::string> &GetSelectedWords() const
    {
        return selectedWords_;
    }

private:
    std::vector<std::string> wordlist_;
    std::vector<std::string> selectedWords_;
    CryptoPP::SecByteBlock seed_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey ecPrivateKey_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey ecPublicKey_;
    std::string privateKey_;
    std::string publicKey_;
    std::map<std::time_t, double> balance_;
    void sendTransaction(const Transaction &transaction);
};

#endif // WALLET_HPP