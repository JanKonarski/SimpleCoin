#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/system_error.hpp>

#include <config.hpp>
#include <network/node.hpp>
#include <network/client.hpp>
#include <blockchain/chain.hpp>

class P2P {
private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::acceptor client_acceptor;
    std::thread io_service_thread;
    std::thread acceptor_thread;
    std::thread client_acceptor_thread;

    std::vector<Node> nodes;
    std::mutex nodes_mutex;
    std::vector<Client> clients;

    Chain& chain;
    std::mutex& chain_mutex;
    std::vector<Transaction>& mem_pool;
    std::mutex& mem_pool_mutex;

    std::function<bool(Block)> addBlock;

    bool freeze = false;

    void startAcceptor() {
        while (true) {
            boost::asio::ip::tcp::socket socket(io_service);
            acceptor.accept(socket);
            Node node(io_service, std::move(socket),
                      std::bind(&P2P::addTransaction, this, std::placeholders::_1),
                      std::bind(&P2P::send, this, std::placeholders::_1),
                      addBlock,
                      std::bind(&P2P::getChain, this),
                      std::bind(&P2P::updateChain, this, std::placeholders::_1));

            std::lock_guard<std::mutex> lock(nodes_mutex);
            nodes.push_back(std::move(node));
            nodes.back().start();
        }
    }

    void startClientAcceptor() {
        while (true) {
            boost::asio::ip::tcp::socket socket(io_service);
            client_acceptor.accept(socket);
            Client client(std::move(socket),
                          std::bind(&P2P::addTransaction, this, std::placeholders::_1),
                          std::bind(&P2P::send, this, std::placeholders::_1),
                          std::bind(&P2P::getTransactions, this, std::placeholders::_1));


            clients.push_back(std::move(client));
            clients.back().start();
        }
    }

public:
    P2P(Chain& chain,
        std::mutex& chain_mutex,
        std::vector<Transaction>& mem_pool,
        std::mutex& mem_pool_mutex,
        std::function<bool(Block)> addBlock)
    : acceptor(io_service, boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(),
            MINER_PORT)),
      client_acceptor(io_service, boost::asio::ip::tcp::endpoint(
              boost::asio::ip::tcp::v4(),
              CLIENT_PORT)),
      chain(chain),
      chain_mutex(chain_mutex),
      mem_pool(mem_pool),
      mem_pool_mutex(mem_pool_mutex),
      addBlock(addBlock)
    {
        nodes.reserve(1024);
        clients.reserve(1024);

        try {
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver ::query query(boost::asio::ip::host_name(), "");
            boost::asio::ip::tcp::resolver::iterator endpoints = resolver.resolve(query);

            while (endpoints != boost::asio::ip::tcp::resolver::iterator()) {
                std::string msg("Host IP: ");
                msg += endpoints->endpoint().address().to_string();
                msg += "\n";

                std::cout.write(msg.c_str(), msg.size());

                ++endpoints;
            }
        }
        catch (const boost::system::system_error& e) {
            std::string msg("Can't find network address\n");
            std::cerr.write(msg.c_str(), msg.size());
            exit(EXIT_FAILURE);
        }

        io_service_thread = std::thread(boost::bind(&boost::asio::io_service::run,&io_service));
        acceptor_thread = std::thread(&P2P::startAcceptor, this);
        client_acceptor_thread = std::thread(&P2P::startClientAcceptor, this);
    }

    void startNetwork() {
        io_service_thread.detach();
        acceptor_thread.detach();
        client_acceptor_thread.detach();
    }

    void connectToNode(boost::asio::ip::address address) {
        boost::asio::ip::tcp::endpoint endpoint(address, MINER_PORT);

        Node node(io_service, endpoint,
                  std::bind(&P2P::addTransaction, this, std::placeholders::_1),
                  std::bind(&P2P::send, this, std::placeholders::_1),
                  addBlock,
                  std::bind(&P2P::getChain, this),
                  std::bind(&P2P::updateChain, this, std::placeholders::_1));

        std::lock_guard<std::mutex> lock(nodes_mutex);
        nodes.push_back(std::move(node));
        nodes.back().start();
    }

    void sleep() {
        freeze = true;
        for (auto& node : nodes)
            node.sleep();

        for (auto& client: clients)
            client.sleep();
    }

    void wakeup() {
        freeze = false;
        for (auto& node : nodes)
            node.wakeup();

        for (auto& client : clients)
            client.wakeup();

        Message message(MessageType::CHAIN_REQUEST, "{}");
        send(message);

        std::this_thread::sleep_for(std::chrono::seconds(2));

//        std::vector<std::pair<std::string, int>> chain_proposals;
//        for (auto& node : nodes) {
//            auto propose = node.getReceivedChain();
//
//            auto propose_it = std::find_if(chain_proposals.begin(),
//                                           chain_proposals.end(),
//                                           [&propose](std::pair<std::string, int> item){
//                return item.first == propose;
//            });
//
//            if (propose_it == chain_proposals.end())
//                chain_proposals.push_back(std::make_pair(propose, 1));
//            else
//                propose_it->second += 1;
//        }
//
//        auto max_it = std::max_element(chain_proposals.begin(),
//                                       chain_proposals.end(),
//                                       [](const auto& a, const auto& b){
//            return a.first.length() < b.first.length();
//        });
//
//        if (max_it != chain_proposals.end())
//            std::cout << "find proposal of chain -> " << max_it->first << std::endl;

        std::unordered_map<std::string, int> chain_proposals;

        for (auto& node : nodes) {
            auto propose = node.getReceivedChain();
            chain_proposals[propose]++;
        }

        auto max_it = std::max_element(
                chain_proposals.begin(),
                chain_proposals.end(),
                [](const auto& a, const auto& b) {
                    return a.first.length() < b.first.length();
                });

        if (max_it != chain_proposals.end()) {
            Chain local_chain(max_it->first);
            if (!local_chain.verify())
                return;

            if (chain.toJson() == local_chain.toJson())
                return;

            std::lock_guard<std::mutex> chain_lock(chain_mutex);
            std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);

// DEBUG
            std::cout << "Find proposal of chain -> " << max_it->first << std::endl;

            for (auto block: chain.chain) {
                for (auto transaction: block.merkle_tree.transactions) {
                    bool exist = false;

                    for (auto local_chain_block: local_chain.chain) {
                        if (exist)
                            continue;

                        auto transaction_it = std::find_if(local_chain_block.merkle_tree.transactions.begin(),
                                                           local_chain_block.merkle_tree.transactions.end(),
                                                           [&transaction](Transaction chain_transaction) {
                                                               return transaction.getHash() ==
                                                                      chain_transaction.getHash();
                                                           });

                        if (transaction_it != local_chain_block.merkle_tree.transactions.end())
                            exist = true;
                    }

                    if (!exist) {
                        transaction.status = false;
                        mem_pool.push_back(transaction);
// DEBUG
                        std::cout << "Transaction returned to mempool -> " << transaction.toJson() << std::endl;
                    } else
// DEBUG
                        std::cout << "Transaction not returned -> " << transaction.toJson() << std::endl;

                }
            }

            chain = local_chain;

            Message chain_message(MessageType::CHAIN_SYNC, chain.toJson());
            send(chain_message);
        }
    }

    bool addTransaction(Transaction transaction) {
        if (transaction.status)
            return false;
        if (!transaction.valid())
            return false;

        {
            std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);
            std::lock_guard<std::mutex> chain_lock(chain_mutex);

            auto mem_pool_it = std::find_if(mem_pool.begin(), mem_pool.end(),
                                            [&transaction](Transaction tr) {
                                                return transaction.getHash() == tr.getHash();
                                            });
            if (mem_pool_it != mem_pool.end())
                return false;

            if (chain.find(transaction))
                return false;

            // TODO
            float money = 100.;

            for (auto block : chain.chain) {
                for (auto block_transaction : block.merkle_tree.transactions) {
//                    if (block_transaction.input_pubkey == block_transaction.output_pubkey)
//                        continue;

                    if (block_transaction.input_pubkey == transaction.input_pubkey) {
                        money -= block_transaction.amount;
                    }
                    else if (block_transaction.output_pubkey == transaction.input_pubkey) {
                        money += block_transaction.amount;
                    }
                }
            }

            for (auto mem_pool_transaction : mem_pool) {
//                    if (mem_pool_transaction.input_pubkey == mem_pool_transaction.output_pubkey)
//                        continue;

                if (mem_pool_transaction.input_pubkey == transaction.input_pubkey) {
                    money -= mem_pool_transaction.amount;
                }
                else if (mem_pool_transaction.output_pubkey == transaction.input_pubkey) {
                    money += mem_pool_transaction.amount;
                }
            }

            if (money < transaction.amount)
                return false;

            mem_pool.push_back(transaction);
        }

        return true;
    }

    void send(Message message) {
        for (auto& node : nodes)
            node.send(message);
    }

    std::string getChain() {
        std::lock_guard<std::mutex> chain_lock(chain_mutex);
        return chain.toJson();
    }

    bool updateChain(std::string chain_json) {
        Chain local_chain(chain_json);

        if (!local_chain.verify())
            return false;

        std::lock_guard<std::mutex> chain_lock(chain_mutex);

        if (chain.toJson() == local_chain.toJson())
            return false;

        std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);

// DEBUG
        std::cout << "Find proposal of chain -> " << chain_json << std::endl;

        for (auto block: chain.chain) {
            for (auto transaction: block.merkle_tree.transactions) {
                bool exist = false;

                for (auto local_chain_block: local_chain.chain) {
                    if (exist)
                        continue;

                    auto transaction_it = std::find_if(local_chain_block.merkle_tree.transactions.begin(),
                                                       local_chain_block.merkle_tree.transactions.end(),
                                                       [&transaction](Transaction chain_transaction) {
                                                           return transaction.getHash() ==
                                                                  chain_transaction.getHash();
                                                       });

                    if (transaction_it != local_chain_block.merkle_tree.transactions.end())
                        exist = true;
                }

                if (!exist) {
                    transaction.status = false;
                    mem_pool.push_back(transaction);
// DEBUG
                    std::cout << "Transaction returned to mempool -> " << transaction.toJson() << std::endl;
                } else
// DEBUG
                    std::cout << "Transaction not returned -> " << transaction.toJson() << std::endl;

            }
        }

        chain = local_chain;

        return true;
    }

    std::vector<Transaction> getTransactions(std::string pubkey) {
        std::vector<Transaction> transactions;

        {
            std::lock_guard<std::mutex> chain_lock(chain_mutex);
            for (auto block : chain.chain)
                for (auto transaction : block.merkle_tree.transactions)
                    if (transaction.input_pubkey == pubkey | transaction.output_pubkey == pubkey)
                        transactions.push_back(transaction);
        }
        {
            std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mutex);
            for (auto transaction : mem_pool)
                if (transaction.input_pubkey == pubkey | transaction.output_pubkey == pubkey)
                    transactions.push_back(transaction);
        }

        return transactions;
    }
};
