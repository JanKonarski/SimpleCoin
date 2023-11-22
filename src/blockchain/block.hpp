#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

class Block {
private:
    std::string previous_hash = "0000000000000000000000000000000000000000000000000000000000000000";
    std::string merkle_hash = "7d8cc9cfffdbd88014588602d42cc8bb58704966b5c5454deb14499b5a9f0207";
    unsigned int time;
    unsigned int difficulty = 5;
    unsigned int nonce = 0;

    int countZeros(std::string text) {
        return std::count(text.begin(), std::find_if(text.begin(),
                                                     text.end(),
                                                     [](char c){
                                                         return c != '0';
                                                     }), '0');
    }

public:

    /* New block */
    Block()
    {
        auto now = std::chrono::system_clock::now();
        time = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    void setPrehash(std::string pre_hash) {
        previous_hash = pre_hash;
    }

    /* Block from network worker */
    Block(std::string str_block) {
        previous_hash = str_block.substr(0, 64);
        merkle_hash = str_block.substr(64, 64);

        size_t pos;

        time = std::stoul(str_block.substr(128, 8), &pos, 16);
        difficulty = std::stoul(str_block.substr(136, 8), &pos, 16);
        nonce = std::stoul(str_block.substr(144, 8), &pos, 16);
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
        while (countZeros(getHash()) != difficulty)
            ++nonce;
    }

    std::string toString() {
        std::stringstream ss;
        ss << previous_hash << merkle_hash <<
           std::setfill('0') << std::setw(8) << std::hex << time <<
           std::setfill('0') << std::setw(8) << std::hex << difficulty <<
           std::setfill('0') << std::setw(8) << std::hex <<nonce;

        return ss.str();
    }

    std::string getPrevious() {
        return previous_hash;
    }

    std::string getMerkle() {
        return merkle_hash;
    }

    unsigned int getTime() {
        return time;
    }

    unsigned int getDifficulty() {
        return difficulty;
    }

    unsigned int getNonce() {
        return nonce;
    }
};
