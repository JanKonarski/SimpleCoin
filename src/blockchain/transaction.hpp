#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/ecp.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <boost/property_tree/json_parser.hpp>

class Transaction {
public:
    std::string input_pubkey;
    std::string output_pubkey;
    float amount;
    std::time_t timestamp;
    std::string signature;
    bool status = false;

    std::string createDataStringForSigning() {
        std::string data = input_pubkey + std::to_string(amount)
                + output_pubkey + std::to_string(timestamp);
        return data;
    }

public:
    Transaction(std::string input_pubkey, float amount, std::string output_pubkey)
    : input_pubkey(input_pubkey), // hex_string
      output_pubkey(output_pubkey), // hex_string
      amount(amount),
      timestamp(std::time(nullptr))
    {}

    Transaction(std::string encode_input_pubkey, float amount, std::string encode_output_pubkey,
                std::string signature, long timestamp, bool status)
    : amount(amount),
      signature(signature),
      timestamp(timestamp),
      status(status)
    {
        CryptoPP::StringSource(encode_input_pubkey, true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(input_pubkey)
            )
        );

        CryptoPP::StringSource(encode_output_pubkey, true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(output_pubkey)
            )
        );
    }

    Transaction(std::string json) {
        boost::property_tree::ptree pt;
        std::istringstream is(json);
        boost::property_tree::read_json(is, pt);

        CryptoPP::StringSource(pt.get<std::string>("input"), true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(input_pubkey)
            )
        );

        CryptoPP::StringSource(pt.get<std::string>("output"), true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(output_pubkey)
            )
        );

//        CryptoPP::StringSource(pt.get<std::string>("signature"), true,
//            new CryptoPP::Base64Decoder(
//                new CryptoPP::StringSink(signature)
//            )
//        );
        signature = pt.get<std::string>("signature");

        amount = pt.get<float>("amount");
        timestamp = pt.get<long>("timestamp");
        status = pt.get<bool>("status");
    }

    std::string getHash() {
        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        auto data = createDataStringForSigning();
        hash.CalculateDigest(digest, (CryptoPP::byte*)data.c_str(), data.length());

        CryptoPP::HexEncoder encoder;
        std::string output;
        encoder.Attach(new CryptoPP::StringSink(output));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        return output;
    }

    void sign(std::string private_key) {
        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
        CryptoPP::ByteQueue queue;
        queue.Put(reinterpret_cast<const CryptoPP::byte *>(private_key.data()), private_key.size());
        queue.MessageEnd();
        privateKey.Load(queue);

        std::string data = createDataStringForSigning() + getHash();
        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, reinterpret_cast<const CryptoPP::byte *>(data.data()), data.size());

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

    bool valid() {
        if (input_pubkey == output_pubkey)
            return false;

        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
        CryptoPP::ByteQueue queue;
        queue.Put(reinterpret_cast<const CryptoPP::byte *>(&input_pubkey[0]), input_pubkey.size());
        publicKey.Load(queue);

        std::string data = createDataStringForSigning() + getHash();
        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, reinterpret_cast<const CryptoPP::byte *>(data.data()),
                             data.size());



        std::string rawSignature;
        CryptoPP::StringSource(signature, true,
            new CryptoPP::HexDecoder(
                new CryptoPP::StringSink(rawSignature)
            )
        );

        CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(publicKey);
        return verifier.VerifyMessage(digest, sizeof(digest), (CryptoPP::byte*)rawSignature.c_str(), rawSignature.length());
    }

    std::string toJson() {
        boost::property_tree::ptree pt;

        std::string encoded_input_pubkey;
        CryptoPP::StringSource(input_pubkey, true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(encoded_input_pubkey),
                false
            )
        );
        pt.put("input", encoded_input_pubkey);

        std::string encoded_output_pubkey;
        CryptoPP::StringSource(output_pubkey, true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(encoded_output_pubkey),
                false
            )
        );
        pt.put("output", encoded_output_pubkey);

//        std::string encoded_signature;
//        CryptoPP::StringSource(signature, true,
//                               new CryptoPP::Base64Encoder(
//                                       new CryptoPP::StringSink(encoded_signature)
//                               )
//        );
//        pt.put("signature", encoded_signature);
        pt.put("signature", signature);

        pt.put("amount", amount);
        pt.put("timestamp", timestamp);
        pt.put("status", status);

        std::ostringstream os;
        boost::property_tree::json_parser::write_json(os, pt);

        return os.str();
    }
};
