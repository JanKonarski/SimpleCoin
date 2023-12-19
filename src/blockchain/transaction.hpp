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
#include <boost/json.hpp>

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

    Transaction(const std::string in_pubkey, const double amount, const std::string out_pubkey)
    {
        input.pubkey = in_pubkey;
        input.amount = amount;
        Output output;
        output.pubkey = out_pubkey;
        output.amount = amount;
        outputs.push_back(output);
        timestamp = std::time(nullptr);
    }

    Transaction(const std::string &json)
    {
        boost::json::value jv = boost::json::parse(json);
        boost::json::object jo = jv.as_object();

        input.pubkey = jo["input"].as_object().at("pubkey").as_string().c_str();
        input.amount = jo["input"].as_object().at("amount").as_double();

        boost::json::array outputsArray = jo["outputs"].as_array();
        for (const auto &output : outputsArray)
        {
            Output out;
            out.pubkey = output.as_object().at("pubkey").as_string().c_str();
            out.amount = output.as_object().at("amount").as_double();
            outputs.push_back(out);
        }

        timestamp = std::time(nullptr);
        createDataStringForSigning(); // This will set .hash field of the class
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
