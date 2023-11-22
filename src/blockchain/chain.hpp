#pragma once

#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <blockchain/block.hpp>

class Chain {
public:
    std::vector<Block> chain;

public:
    Chain() {
    }

    Chain(std::string json_chain) {
        std::istringstream iss(json_chain);
        boost::property_tree::ptree ptree;
        boost::property_tree::json_parser::read_json(iss, ptree);

        for (const auto& text_block : ptree.get_child("chain")) {
            std::string text = text_block.second.get_value<std::string>();
            Block block(text);

            if (!add(block))
                throw std::runtime_error("Invalid block in chain import");
        }
    }

    bool add(Block block) {
        if (chain.empty()) {
            if (block.getPrevious() != "0000000000000000000000000000000000000000000000000000000000000000")
                return false;

            chain.push_back(block);
            return true;
        } else if (chain.back().getHash() == block.getPrevious()) {
            chain.push_back(block);
            return true;
        }

        return false;
    }

    std::string toString() {
        boost::property_tree::ptree ptree;

        for (size_t i=0; i<chain.size(); i++) {
            std::string position("chain.block");
            position += i;

            ptree.put(position, chain[i].toString());
        }

        std::ostringstream oss;
        boost::property_tree::json_parser::write_json(oss, ptree);
        return oss.str();
    }

    std::string getLastHash() {
        if (chain.empty())
            return "0000000000000000000000000000000000000000000000000000000000000000";

        else
            return chain.back().getHash();
    }
};
