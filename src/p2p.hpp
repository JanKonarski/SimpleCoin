#pragma once

#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <config.hpp>
#include <peer.hpp>

class P2P {
private:
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    std::shared_ptr<std::vector<std::shared_ptr<Peer>>> nodeList;

    void startAccept() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioService);

        acceptor.async_accept(*socket,
                              [this, socket](boost::system::error_code error){
            if (error)
                std::cerr << "Error accepting connection: " << error.message() << std::endl;
            else {
                std::cout << "Connection accepted" << std::endl;
                auto peer = std::make_shared<Peer>(socket, nodeList);
                nodeList->push_back(std::move(peer));
            }

            startAccept();
        });
    }

    void startRead(std::string msg) {
        std::cout << msg << std::endl;
    }

public:
    P2P(boost::asio::io_service& io_service) :
            ioService(io_service),
            acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                                PORT))
    {
        nodeList = std::make_shared<std::vector<std::shared_ptr<Peer>>>();
    }

    void setNodeList(std::shared_ptr<std::vector<std::shared_ptr<Peer>>> newNodeList) {
        nodeList = newNodeList;
    }

    void startListening() {
        startAccept();
    }

    void makeConnection(std::vector<boost::asio::ip::address> addresses) {
        if (addresses.empty())
            return;

        auto address = addresses.back();
        std::cout << address << " ...";
        try {
            boost::asio::ip::tcp::endpoint endpoint(address, PORT);
            auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioService);
            socket->connect(endpoint);
            std::cout << "[o]" << std::endl;

            // TODO: obsługa połączeń
            auto peer = std::make_shared<Peer>(socket, nodeList);
            nodeList->push_back(std::move(peer));
        }
        catch (...) {
            std::cout << "[x]" << std::endl;
        }

        std::vector<boost::asio::ip::address> subAddresses(addresses.begin(),
                                                           addresses.end() - 1);
        makeConnection(subAddresses);
    }
};