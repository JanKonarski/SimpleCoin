#pragma once
#include <iostream>

#include <vector>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <config.hpp>
#include <network/node.hpp>
#include <network/protocol.hpp>
#include <blockchain/chain.hpp>

class P2P {
private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    std::thread io_service_thread;
    std::thread acceptor_thread;
    std::vector<std::shared_ptr<Node>> nodes;
    Chain* chain;

    void createNode(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        auto node = std::make_shared<Node>(std::move(socket));
        node->setNodesList(&nodes);
        node->setChain(chain);

        nodes.push_back(std::move(node));
    }

    void startAccept() {
        while(true) {
            auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service);

            acceptor.accept(*socket);

            createNode(std::move(socket));
        }
    }

public:
    P2P() :
        acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                            PORT))
    {
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

        io_service_thread = std::thread(boost::bind(&boost::asio::io_service::run, &io_service));
        acceptor_thread = std::thread(&P2P::startAccept, this);
    }

    void setChain(Chain* chain_) {
        chain = chain_;
    }

    void startNetwork() {

        io_service_thread.detach();
        acceptor_thread.detach();
    }

    void stopNetwork() {
    }

    void connectToNode(boost::asio::ip::address_v4 address) {
        boost::asio::ip::tcp::endpoint endpoint(address, PORT);
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service);

        try {
            socket->connect(endpoint);

            createNode(std::move(socket));
        }
        catch (const boost::system::system_error& e) {
            std::string msg("Can't connect to ");
            msg += address.to_string();

            std::cerr.write(msg.c_str(), msg.size());
        }
    }

    void disconnectFromNode() {
    }

    void broadcastMessage(Message message) {
        for (const auto& node : nodes) {
            node->send(message);
        }
    }

    void lastNodeGetNodesList() {
        nodes.back()->nodesListRequest();
    }
};