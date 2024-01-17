#pragma once

#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <config.hpp>
#include <network/protocol.hpp>
#include <blockchain/block.hpp>
#include <blockchain/chain.hpp>
#include <blockchain/transaction.hpp>

class Node {
private:
    boost::asio::ip::tcp::socket sender;
    boost::asio::ip::tcp::socket receiver;
    std::thread receive_thread;

    std::function<bool(Transaction)> addTransaction;
    std::function<void(Message)> p2pSend;
    std::function<bool(Block)> addBlock;
    std::function<std::string()> getChain;
    std::function<bool(std::string)> updateChain;

    std::string received_chain_json;

    bool freeze = false;

    void startReceive() {
        while (true) {
            Message request;
            receiver.receive(boost::asio::buffer(&request, sizeof(Message)));

            boost::property_tree::ptree request_pt;
            std::istringstream is(request.data);
            boost::property_tree::json_parser::read_json(is, request_pt);

//            if (freeze)
//                continue;

            if (request.type == MessageType::TRANSACTION_TRANSFER) {
                if (freeze)
                    continue;

                for (auto item : request_pt.get_child("transactions")) {
                    Transaction transaction(item.second.get<std::string>("input"),
                                            item.second.get<float>("amount"),
                                            item.second.get<std::string>("output"),
                                            item.second.get<std::string>("signature"),
                                            item.second.get<long>("timestamp"),
                                            item.second.get<bool>("status"));

                    if (addTransaction(transaction)) {
// DEBUG
std::cout << "Receive transaction from miner -> " << transaction.toJson() << std::endl;

                        boost::property_tree::ptree root_tr;
                        boost::property_tree::ptree array_tr;
                        boost::property_tree::ptree ptree_tr;
                        std::istringstream is_tr(transaction.toJson());
                        boost::property_tree::json_parser::read_json(is_tr, ptree_tr);
                        array_tr.push_back(std::make_pair("", ptree_tr));
                        root_tr.put_child("transactions", array_tr);

                        std::ostringstream os_tr;
                        boost::property_tree::json_parser::write_json(os_tr, root_tr);

                        Message message(MessageType::TRANSACTION_TRANSFER, os_tr.str());
                        p2pSend(message);
                    }
                }
            }
            else if (request.type == MessageType::BLOCK_TRANSFER) {
                if (freeze)
                    continue;

                for (auto item : request_pt.get_child("blocks")) {
                    std::ostringstream os;
                    boost::property_tree::json_parser::write_json(os, item.second);

                    Block block(os.str());

                    if (addBlock(block)) {
// DEBUG
std::cout << "Added receive block -> " << block.toJson() << std::endl;

                        boost::property_tree::ptree root_ptree;
                        boost::property_tree::ptree array_ptree;
                        array_ptree.push_back(std::make_pair("", item.second));
                        root_ptree.put_child("blocks", array_ptree);

                        std::ostringstream os;
                        boost::property_tree::json_parser::write_json(os, root_ptree);

                        Message message(MessageType::BLOCK_TRANSFER, os.str());
                        p2pSend(message);
                    }
                }
            }
            else if (request.type == MessageType::CHAIN_REQUEST) {
                if (getChain().length() < 10)
                    continue;

                Message message(MessageType::CHAIN_TRANSFER, getChain());
                sender.send(boost::asio::buffer(&message, sizeof(Message)));
            }
            else if (request.type == MessageType::CHAIN_TRANSFER) {
                std::string receive_json(request.data);
                if (received_chain_json == receive_json)
                    continue;

                Chain local_chain(receive_json);
                if (!local_chain.verify())
                    continue;

                received_chain_json = receive_json;
            }
            else if (request.type == MessageType::CHAIN_SYNC) {
                if (updateChain(request.data)) {
                    p2pSend(request);
                }
            }
        }
    }

public:
    // Initialize connection
    Node(boost::asio::io_service& io_service,
         boost::asio::ip::tcp::endpoint endpoint,
         std::function<bool(Transaction)> addTransaction,
         std::function<void(Message)> send,
         std::function<bool(Block)> addBlock,
         std::function<std::string()> getChain,
         std::function<bool(std::string)> updateChain)
    : sender(io_service),
      receiver(io_service),
      addTransaction(addTransaction),
      p2pSend(send),
      addBlock(addBlock),
      getChain(getChain),
      updateChain(updateChain)
    {
        sender.connect(endpoint);

        boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(
                boost::asio::ip::tcp::v4(),
                MINER_PORT + 1
            ));

        acceptor.accept(receiver);
    }

    // Receive connection
    Node(boost::asio::io_service& io_service,
         boost::asio::ip::tcp::socket socket,
         std::function<bool(Transaction)> addTransaction,
         std::function<void(Message)> send,
         std::function<bool(Block)> addBlock,
         std::function<std::string()> getChain,
         std::function<bool(std::string)> updateChain)
    : sender(io_service),
      receiver(std::move(socket)),
      addTransaction(addTransaction),
      p2pSend(send),
      addBlock(addBlock),
      getChain(getChain),
      updateChain(updateChain)
    {
        boost::asio::ip::tcp::endpoint endpoint(receiver.remote_endpoint().address(), MINER_PORT + 1);
        sender.connect(endpoint);
    }

    void start() {
        receive_thread = std::thread(&Node::startReceive, this);
        receive_thread.detach();
    }

    void sleep() { freeze = true; }
    void wakeup() { freeze = false; }

    void send(Message message) {
        sender.send(boost::asio::buffer(&message, sizeof(Message)));
    }

    std::string getReceivedChain() {
        return received_chain_json;
    }
};
