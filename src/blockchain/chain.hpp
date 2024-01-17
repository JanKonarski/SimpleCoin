#pragma once

#include <string>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <boost/property_tree/json_parser.hpp>

#include <blockchain/block.hpp>

class Chain {
public:
    std::vector<Block> chain;

public:
    Chain() {}

    Chain(std::string json) {
        std::istringstream is(json);
        boost::property_tree::ptree ptree;
        boost::property_tree::json_parser::read_json(is, ptree);

        for (auto item : ptree.get_child("blocks")) {
            boost::property_tree::ptree block_ptree = item.second;
            std::ostringstream block_json;
            boost::property_tree::json_parser::write_json(block_json, block_ptree);

            Block block(block_json.str());
            chain.push_back(block);
        }
    }

    std::string toJson() {
        boost::property_tree::ptree ptree;

        boost::property_tree::ptree array;
        for (auto block : chain) {
            std::istringstream block_json(block.toJson());
            boost::property_tree::ptree block_ptree;
            boost::property_tree::json_parser::read_json(block_json, block_ptree);
            array.push_back(std::make_pair("", block_ptree));
        }
        ptree.put_child("blocks", array);

        std::ostringstream os;
        boost::property_tree::json_parser::write_json(os, ptree);

        return os.str();
    }

    std::string getHash() {
        std::string data;
        for (auto block : chain)
            data += block.getHash();

        CryptoPP::SHA256 sha256;

        std::string hash;
        CryptoPP::StringSource(data, true,
            new CryptoPP::HashFilter(sha256,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(hash))));

        std::transform(hash.begin(), hash.end(), hash.begin(), ::tolower);

        return hash;
    }

    bool add(Block block) {
        if (!block.verify())
            return false;

        if (chain.empty()) {
            if (block.previous_hash != "0000000000000000000000000000000000000000000000000000000000000000")
                return false;

            chain.push_back(block);
            return true;
        }

        if (chain.back().getHash() != block.previous_hash)
            return false;

        for (auto transaction : block.merkle_tree.transactions) {
            if (!transaction.valid())
                return false;

            for (auto chain_block : chain) {
                auto chain_it = std::find_if(chain_block.merkle_tree.transactions.begin(),
                                             chain_block.merkle_tree.transactions.end(),
                                             [&transaction](Transaction chain_transaction){
                                                 return transaction.getHash() == chain_transaction.getHash();
                                             });

                if (chain_it != chain_block.merkle_tree.transactions.end())
                    return false;
            }
        }

        chain.push_back(block);
        return true;
    }

    bool verify() {
        for (auto block : chain)
            if (!block.verify())
                return false;

        return true;
    }

    bool find(Transaction transaction) {
        auto block_it = std::find_if(chain.begin(), chain.end(),
                                     [&transaction](Block block){
            return block.find(transaction);
        });

        return block_it != chain.end();
    }

    std::string getLast() {
        std::string last = "0000000000000000000000000000000000000000000000000000000000000000";

        if (!chain.empty())
            last = chain.back().getHash();

        return last;
    }
};