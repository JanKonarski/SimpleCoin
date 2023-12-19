#ifndef DIGITAL_IDENTITY_HPP
#define DIGITAL_IDENTITY_HPP

#include <string>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/ecp.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>

class DigitalIdentity
{
public:
    DigitalIdentity(const std::string &wordlistFileName);
    ~DigitalIdentity();

    bool LoadWordList(const std::string &filename);
    bool SelectRandomWords(size_t count);
    bool DeriveSeedFromWords();
    bool GenerateECDSAKeyPair();

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
};

#endif // DIGITAL_IDENTITY_HPP
