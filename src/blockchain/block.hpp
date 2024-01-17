#pragma once

#include <string>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <config.hpp>
#include <blockchain/merkle.hpp>

class Block {
public:
    std::string previous_hash;
    Merkle merkle_tree;
    std::time_t timestamp;
    int difficulty = DIFFICULTY;
    int nonce = 0;

    int countZeros(std::string text) {
        return std::count(text.begin(), std::find_if(text.begin(),
                                                     text.end(),
                                                     [](char c){
                                                         return c != '0';
                                                     }), '0');
    }

public:
    Block(Merkle merkle_tree,
          std::string previous_hash = "0000000000000000000000000000000000000000000000000000000000000000")
    : previous_hash(previous_hash),
      merkle_tree(merkle_tree)
    {
        timestamp = std::time(nullptr);
    };

    Block(std::string json) {
        std::istringstream is(json);
        boost::property_tree::ptree ptree;
        boost::property_tree::json_parser::read_json(is, ptree);

        previous_hash = ptree.get<std::string>("previous");
        timestamp = ptree.get<long>("timestamp");
        difficulty = ptree.get<int>("difficulty");
        nonce = ptree.get<int>("nonce");

        boost::property_tree::ptree merkle_ptree = ptree.get_child("merkle");
        std::ostringstream merkle_json;
        boost::property_tree::json_parser::write_json(merkle_json, merkle_ptree);
        merkle_tree = Merkle(merkle_json.str());
    }

    std::string getHash() {
        std::string block_text = toString();
        std::string hash;

        CryptoPP::SHA256 sha256;

        CryptoPP::StringSource(block_text, true,
            new CryptoPP::HashFilter(sha256,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(hash))));

        std::transform(hash.begin(), hash.end(), hash.begin(), ::tolower);

        return hash;
    }

    void mineBlock() {
        while (countZeros(getHash()) != DIFFICULTY)
            ++nonce;
    }

    std::string toString() {
        std::stringstream ss;
        ss << previous_hash << merkle_tree.getHash() <<
           std::setfill('0') << std::setw(8) << std::hex << time <<
           std::setfill('0') << std::setw(8) << std::hex << difficulty <<
           std::setfill('0') << std::setw(8) << std::hex <<nonce;

        return ss.str();
    }

    bool verify() {
        return countZeros(getHash()) == difficulty;
    }

    std::string toJson() {
        boost::property_tree::ptree ptree;

        ptree.put("previous", previous_hash);
        ptree.put("timestamp", timestamp);
        ptree.put("difficulty", difficulty);
        ptree.put("nonce", nonce);

        boost::property_tree::ptree merkle_ptree;
        std::istringstream is(merkle_tree.toJson());
        boost::property_tree::json_parser::read_json(is, merkle_ptree);
        ptree.put_child("merkle", merkle_ptree);

        std::ostringstream os;
        boost::property_tree::json_parser::write_json(os, ptree);

        return os.str();
    }

    bool find(Transaction transaction) {
        return merkle_tree.find(transaction);
    }
};