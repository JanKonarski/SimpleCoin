#pragma once

#include <string>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <boost/property_tree/json_parser.hpp>

#include <blockchain/transaction.hpp>

class Merkle {
public:
    std::vector<Transaction> transactions;
    std::string root_hash;

public:
    Merkle() {}

    Merkle(std::vector<Transaction> transactions)
    : transactions(transactions)
    {
        root_hash = getHash();
    }

    Merkle(std::string json) {
        std::istringstream is(json);
        boost::property_tree::ptree pt;
        boost::property_tree::json_parser::read_json(is, pt);

        root_hash = pt.get<std::string>("hash");

        for (auto item : pt.get_child("transactions")) {
            Transaction transaction(item.second.get<std::string>("input"),
                                    item.second.get<float>("amount"),
                                    item.second.get<std::string>("output"),
                                    item.second.get<std::string>("signature"),
                                    item.second.get<long>("timestamp"),
                                    item.second.get<bool>("status"));

            transactions.push_back(transaction);
        }
    }

    std::string getHash() {
        std::string sum_hash;

        for (auto transaction : transactions)
            sum_hash += transaction.getHash();

        CryptoPP::SHA256 hash;
        CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, (CryptoPP::byte*)sum_hash.c_str(), sum_hash.length());

        CryptoPP::HexEncoder encoder;
        std::string output;
        encoder.Attach(new CryptoPP::StringSink(output));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        return output;
    }

    bool valid() {
        return root_hash == getHash();
    }

    std::string toJson() {
        boost::property_tree::ptree pt;
        pt.put("hash", root_hash);

        boost::property_tree::ptree array;
        for (auto transaction : transactions) {
            std::istringstream is(transaction.toJson());
            boost::property_tree::ptree tr_ptree;
            boost::property_tree::json_parser::read_json(is, tr_ptree);

            array.push_back(std::make_pair("", tr_ptree));
        }
        pt.put_child("transactions", array);

        std::ostringstream os;
        boost::property_tree::json_parser::write_json(os, pt);

        return os.str();
    }

    bool find(Transaction transaction) {
        auto it = std::find_if(transactions.begin(), transactions.end(),
                               [&transaction](Transaction tr){
            return tr.getHash() == transaction.getHash();
        });

        return it != transactions.end();
    }
};