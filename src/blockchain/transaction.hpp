#include <string>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>
#include <cstddef>
#include <cryptopp/rsa.h>
#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>

class Transaction
{
public:
    struct Input
    {
        std::string pubkey;
        double amount;
    };

    struct Output
    {
        std::string pubkey;
        double amount;
    };

    Transaction(const Input &input, const std::vector<Output> &outputs)
        : input(input), outputs(outputs), timestamp(std::time(nullptr))
    {
    }

    std::string getHash(std::string data) const
    {
        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, (CryptoPP::byte *)data.c_str(), data.length());

        CryptoPP::HexEncoder encoder;
        std::string output;
        encoder.Attach(new CryptoPP::StringSink(output));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        return output;
    }

    void sign(const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privateKey)
    {
        std::string dataToBeSigned = createDataStringForSigning();
        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, reinterpret_cast<const CryptoPP::byte *>(dataToBeSigned.data()), dataToBeSigned.size());

        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer signer(privateKey);
        size_t sigLen = signer.MaxSignatureLength();
        std::string rawSignature;
        rawSignature.resize(sigLen);
        sigLen = signer.SignMessage(rng, digest, sizeof(digest), reinterpret_cast<CryptoPP::byte *>(&rawSignature[0]));
        rawSignature.resize(sigLen);
        CryptoPP::StringSource ss(rawSignature, true,
                                  new CryptoPP::HexEncoder(
                                      new CryptoPP::StringSink(signature)));
    }

    std::string toJson() const
    {
        std::string json = "{";

        json += "\"input\": {";
        json += "\"pubkey\": \"" + input.pubkey + "\",";
        json += "\"amount\": " + std::to_string(input.amount);
        json += "},";

        json += "\"outputs\": [";
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            json += "{";
            json += "\"pubkey\": \"" + outputs[i].pubkey + "\",";
            json += "\"amount\": " + std::to_string(outputs[i].amount);
            json += "}";
            if (i < outputs.size() - 1)
                json += ",";
        }
        json += "],";

        json += "\"timestamp\": " + std::to_string(timestamp) + ",";
        json += "\"hash\": \"" + hash + "\",";
        json += "\"signature\": \"" + signature + "\"";

        json += "}";

        return json;
    }
    Input input;
    std::vector<Output> outputs;

private:
    std::string createDataStringForSigning()
    {
        std::string data = input.pubkey + std::to_string(input.amount);
        for (const auto &output : outputs)
        {
            data += output.pubkey + std::to_string(output.amount);
        }
        hash = getHash(data);
        data += hash;
        data += std::to_string(timestamp);
        return data;
    }
    std::time_t timestamp;
    std::string signature;
    std::string hash;
};
