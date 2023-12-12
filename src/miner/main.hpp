#pragma once

#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <boost/program_options.hpp>

#include <network/p2p.hpp>
#include <blockchain/block.hpp>
#include <blockchain/chain.hpp>

class Main {
private:
    P2P p2p;

    void arguments(std::vector<std::string> args) {
        std::vector<const char*> char_args;
        std::transform(args.begin(), args.end(), std::back_inserter(char_args),
                       [](const std::string& str){return str.c_str();});

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("node", boost::program_options::value<std::vector<std::string>>()->multitoken(),
                        "node addresses");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(char_args.size(),
                                                                                 char_args.data(),
                                                                                 desc),
                                      vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::ostringstream oss;
            desc.print(oss);
            std::cout.write(oss.str().c_str(), oss.str().size());

            exit(EXIT_SUCCESS);
        }

        if (!vm["node"].empty()) {
            try {
                for (auto str_address : vm["node"].as<std::vector<std::string>>()) {
                    auto address = boost::asio::ip::address_v4::from_string((str_address));

                    p2p.connectToNode(address);

//                    p2p.lastNodeGetNodesList();
                }
            }
            catch (const boost::system::system_error& e) {
//                std::string msg("Arguent address error\n");
                std::string msg("Exception: ");
                msg += e.what();
                msg += "\n";
                std::cerr.write(msg.c_str(), msg.size());

                exit(EXIT_FAILURE);
            }
        }
    }

public:
    Main(const std::vector<std::string> args) {
        p2p.startNetwork();

        Chain chain;
        p2p.setChain(&chain);

        arguments(args);

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<int> distribution(15, 150);

        std::this_thread::sleep_for(std::chrono::seconds(10));

        while (args.size() == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(distribution(generator)));

            Block block;
            block.setPrehash(chain.getLastHash());
            block.mineBlock();

            // grab transactions from mempool
            // add them to block
            // calculate merkle root
            // add merkle root to block
            // broadcast block

            if (chain.add(block))
                std::cout << block.toString() << std::endl;

            Message msg;
            msg.type = Message::BLOCK;
            std::memcpy(msg.data, block.toString().c_str(), block.toString().size());
            p2p.broadcastMessage(msg);
        }

        while (getchar());
    }
};
