#pragma once

#include <array>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <boost/asio.hpp>

#include <config.hpp>

class Server {
private:
    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>>& connections;

    void startAccept() {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket(new boost::asio::ip::tcp::socket(io_service));

        acceptor.accept(*socket);

        connections.push_back(std::move(socket));

        auto read_socket = connections.back();
        auto read_thread = new std::thread([this, read_socket](){
            startRead(read_socket);
        });
        read_thread->detach();
    }

    void startRead(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        try {
            std::array<char, 1024> buffer;
            std::size_t bytesRead = boost::asio::read(*socket, boost::asio::buffer(buffer));
            std::string message(buffer.begin(), buffer.begin() + bytesRead);
            std::cout << "Message: " << message << std::endl << std::endl;
        }
        catch (std::exception& e) {
            std::cerr << "read exception: " << e.what() << std::endl;
        }

        startRead(socket);
    }

public:
    Server(boost::asio::io_service &io_service,
           std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>>& connections) :
            io_service(io_service),
            acceptor(io_service,
                     boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                    PORT)),
            connections(connections)
    {
        std::thread accept_thread([this](){
            while (true)
                startAccept();
        });
        accept_thread.detach();
    }

    void send(std::string msg) {
        msg.resize(1024);
        for (auto connection: connections)
            boost::asio::write(*connection, boost::asio::buffer(msg));
    }
};
