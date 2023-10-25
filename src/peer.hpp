#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <condition_variable>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Peer {
private:
    std::shared_ptr<std::vector<std::shared_ptr<Peer>>> nodeList;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
    std::function<void(std::string msg)> callback;
    std::condition_variable cv;
    std::vector<char> buffer;
    std::string responseText;
    bool ready = false;
    std::mutex mtx;

public:
    void send(std::string controlMsg) {
        socket->send(boost::asio::buffer(controlMsg));
    }

    void startReceive() {
        buffer = std::vector<char>(1024);
        socket->async_receive(boost::asio::buffer(buffer),
                              boost::bind(&Peer::receiveHandle, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    }

    void receiveHandle(boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::string receive(buffer.data(), length);
            responseText = receive;
            boost::property_tree::ptree pt;
            std::istringstream iss(receive);
            boost::property_tree::json_parser::read_json(iss, pt);

            boost::property_tree::ptree response;
            response.put("ACTION", "response");

            auto action = pt.get<std::string>("ACTION");

            if (action == "response") {
                std::cout << receive << std::endl;
                ready = true;
            }
            else if (action == "show") {
                std::cout << "[network] " << pt.get<std::string>("text") << std::endl;
            }
            else if (action == "get-node_list") {
                for (int i=0; i<nodeList->size(); i++) {
                    auto address = (*nodeList)[i]->getAddress();

                    std::string position("items.value");
                    position += std::to_string(i+1);
                    response.put(position, address.to_string());
                }

                std::ostringstream oss;
                boost::property_tree::json_parser::write_json(oss, response);
                std::string responseMsg = oss.str();
                send(responseMsg);
            }
        }
        else if (ec == boost::asio::error::eof) {
            //TODO: obsługa przerwanego połączenia
            std::cout << "ASIO koniec" << std::endl;
        }
        else {
            //TODO: obsługa innego błędu
            std::cout << "ASIO inne" << std::endl;
        }

        startReceive();
    }

public:
    Peer(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
         std::shared_ptr<std::vector<std::shared_ptr<Peer>>> nodeList) :
            socket(socket),
            nodeList(nodeList)
    {
        startReceive();
    }

    bool check() {
        return socket->is_open();
    }

    [[nodiscard]] std::shared_ptr<boost::asio::ip::tcp::socket>
    getSocket() {
        return socket;
    }

    boost::asio::ip::address
    getAddress() {
        return socket->remote_endpoint().address();
    }

    std::vector<boost::asio::ip::address>
    getNodeList () {
        boost::property_tree::ptree controlMSG;
        controlMSG.put("ACTION", "get-node_list");

        std::ostringstream oss;
        boost::property_tree::json_parser::write_json(oss, controlMSG);
        std::string msg = oss.str();

        send(msg);
        while(responseText.empty())
            continue;

//        std::string response(buffer.begin(), buffer.end());
        std::string response = responseText;

        boost::property_tree::ptree pt;
        std::istringstream iss(response);
        boost::property_tree::json_parser::read_json(iss, pt);

        std::vector<boost::asio::ip::address> addressList;
        for (const auto& item : pt.get_child("items")) {
            std::cout << item.second.get_value<std::string>() << std::endl;
            try {
                boost::asio::ip::address address = boost::asio::ip::address::from_string(
                        item.second.get_value<std::string>());
                addressList.push_back(address);
            }
            catch (...) {}
        }
std::cout << addressList.size() << std::endl;
        return addressList;
    }
};
