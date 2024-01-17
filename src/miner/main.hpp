#pragma once

#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <chrono>
#include <random>
#include <random>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <network/p2p.hpp>
#include <blockchain/block.hpp>
#include <blockchain/chain.hpp>
#include <blockchain/transaction.hpp>

class Main{
private:
    P2P p2p;
    Chain chain;
    std::mutex chain_mutex;
    std::vector<Transaction> mem_pool;
    std::mutex mem_pool_mutex;

    std::thread mine_thread;

    bool freeze = false;

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

    void mine() {
        std::random_device rd;
        std::mt19937 gen(rd());

        while (true) {
            if (mem_pool.empty() | freeze) {
                std::uniform_int_distribution<int> dist(MIN_WAIT_RAND, MAX_WAIT_RAND);
                int wait_time = dist(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
                continue;
            }

            if (freeze)
                continue;

            Merkle merkle;
            {
                std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);
                merkle = Merkle(mem_pool);
                mem_pool.clear();
            }

            Block block(merkle, chain.getLast());
            block.mineBlock();

            if (freeze) {
                std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);
                mem_pool = block.merkle_tree.transactions;
                continue;
            }

            for (auto& transaction : block.merkle_tree.transactions)
                transaction.status = true;

            if (addBlock(block)) {
// DEBUG
std::cout << "Added mined block -> " << block.toJson() << std::endl;

                boost::property_tree::ptree root_ptree;
                boost::property_tree::ptree array_ptree;
                std::istringstream is(block.toJson());
                boost::property_tree::ptree block_ptree;
                boost::property_tree::json_parser::read_json(is, block_ptree);

                array_ptree.push_back(std::make_pair("", block_ptree));
                root_ptree.add_child("blocks", array_ptree);

                std::ostringstream os;
                boost::property_tree::json_parser::write_json(os, root_ptree);

                Message message(MessageType::BLOCK_TRANSFER, os.str());
                p2p.send(message);
            }
            else {
// DEBUG
std::cout << "Not added mined block" << std::endl;

                std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);
                std::lock_guard<std::mutex> chain_lock(chain_mutex);

                for (auto transaction : merkle.transactions) {
                    bool exist = false;

                    for (auto chain_block : chain.chain) {
                        if (exist)
                            continue;

                        auto transaction_it = std::find_if(chain_block.merkle_tree.transactions.begin(),
                                                           chain_block.merkle_tree.transactions.end(),
                                                           [&transaction](Transaction chain_transaction){
                            return transaction.getHash() == chain_transaction.getHash();
                        });

                        if (transaction_it != chain_block.merkle_tree.transactions.end())
                            exist = true;
                    }

                    if (!exist) {
                        transaction.status = false;
                        mem_pool.push_back(transaction);
// DEBUG
std::cout << "Transaction returned to mempool -> " << transaction.toJson() << std::endl;
                    }
                    else
// DEBUG
std::cout << "Transaction not returned -> " << transaction.toJson() << std::endl;
                }
            }
        }
    }

    bool addBlock(Block block) {
        if (!block.verify())
            return false;

        {
            std::lock_guard<std::mutex> chain_lock(chain_mutex);
            if (!chain.add(block))
                return false;
        }

        return true;
    }

    void removeFromMemPool(Transaction transaction) {
        for (auto chain_block : chain.chain) {
            for (auto chain_transaction : chain_block.merkle_tree.transactions) {
                mem_pool.erase(std::remove_if(mem_pool.begin(), mem_pool.end(),
                                              [&transaction](Transaction chain_transaction){
                    return transaction.getHash() == chain_transaction.getHash();
                }),
                               mem_pool.end());
            }
        }
    }

public:
    Main(const std::vector<std::string> args)
    : p2p(chain, chain_mutex, mem_pool, mem_pool_mutex,
          std::bind(&Main::addBlock, this, std::placeholders::_1))
    {
        p2p.startNetwork();

        arguments(args);

        mine_thread = std::thread(&Main::mine, this);
        mine_thread.detach();

        std::cout << "Press enter to freeze miner" << std::endl;

        while (true) {
            getchar();

            freeze = !freeze;
            if (freeze)  {
                p2p.sleep();
                std::cout << "Minner freezed" << std::endl;
            }
            else {
                std::cout << "Minner unfreezed" << std::endl;
                p2p.wakeup();
            }
        }
    }
};
