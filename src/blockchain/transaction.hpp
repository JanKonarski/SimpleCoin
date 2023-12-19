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

    Transaction(const Input &inputs, const std::vector<Output> &outputs)
        : input(input), outputs(outputs), timestamp(std::time(nullptr))
    {
    }

    std::string getHash() const
    {
        std::string data = input.pubkey + std::to_string(input.amount);
        for (const auto &output : outputs)
        {
            data += output.pubkey + std::to_string(output.amount);
        }

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

    std::string sign(const CryptoPP::RSA::PrivateKey &privateKey)
    {
        std::string transactionData = createDataStringForSigning();

        CryptoPP::RSASSA_PKCS1v15_SHA_Signer signer(privateKey);

        std::string signature;

        CryptoPP::AutoSeededRandomPool rng;

        CryptoPP::StringSource ss(transactionData, true,
                                  new CryptoPP::SignerFilter(rng, signer,
                                                             new CryptoPP::StringSink(signature)));
        this->signature = signature;
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
        json += "\"signature\": \"" + signature + "\"";

        json += "}";

        return json;
    }

private:
    std::string createDataStringForSigning() const
    {
        std::string data = input.pubkey + std::to_string(input.amount);
        for (const auto &output : outputs)
        {
            data += output.pubkey + std::to_string(output.amount);
        }
        data += std::to_string(timestamp); // Include the timestamp in the signing data
        return data;
    }

    Input input;
    std::vector<Output> outputs;
    std::time_t timestamp;
    std::string signature;
};
