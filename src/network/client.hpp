#pragma once

#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <config.hpp>
#include <network/protocol.hpp>
#include <blockchain/chain.hpp>
#include <blockchain/transaction.hpp>

class Client {
private:
    boost::asio::ip::tcp::socket socket;
    std::thread receiver_thread;

    std::function<bool(Transaction)> addTransaction;
    std::function<void(Message)> send;
    std::function<std::vector<Transaction>(std::string)> getTransactions;

    bool freeze = false;

    void startReceive() {
        while (true) {
            Message request;

            socket.receive(boost::asio::buffer(&request, sizeof(Message)));

            boost::property_tree::ptree request_pt;
            std::istringstream is(request.data);
            boost::property_tree::json_parser::read_json(is, request_pt);

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
std::cout << "Receive transaction from client -> " << transaction.toJson() << std::endl;

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
                        send(message);
                    }
                }
            }
            else if (request.type == MessageType::TRANSACTION_REQUEST) {
                if (freeze) {
                    Message response(MessageType::NULL_RESPONSE);
                    socket.send(boost::asio::buffer(&response, sizeof(Message)));
                    continue;
                }

                Transaction request_tr(request.data);

                boost::property_tree::ptree root_response;
                boost::property_tree::ptree array_response;
                for (auto transaction : getTransactions(request_tr.input_pubkey)) {
                    std::istringstream is(transaction.toJson());
                    boost::property_tree::ptree tr_response;
                    boost::property_tree::json_parser::read_json(is, tr_response);

                    array_response.push_back(std::make_pair("", tr_response));
                }
                root_response.put_child("transactions", array_response);

                std::ostringstream os;
                boost::property_tree::json_parser::write_json(os, root_response);

                Message response(MessageType::TRANSACTION_TRANSFER, os.str());
                socket.send(boost::asio::buffer(&response, sizeof(Message)));
            }
        }
    }

public:
    Client(boost::asio::ip::tcp::socket socket,
           std::function<bool(Transaction)> addTransaction,
           std::function<void(Message)> send,
           std::function<std::vector<Transaction>(std::string)> getTransactions)
    : socket(std::move(socket)),
      addTransaction(addTransaction),
      send(send),
      getTransactions(getTransactions)
    {
    }

    void start() {
        receiver_thread = std::thread(&Client::startReceive, this);
        receiver_thread.detach();
    }



    void sleep() { freeze = true; }
    void wakeup() { freeze = false; }
};