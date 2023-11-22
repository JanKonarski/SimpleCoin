#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <config.hpp>
#include <network/protocol.hpp>
#include <blockchain/chain.hpp>

class Node {
private:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
    std::thread receive_thread;

    std::vector<std::shared_ptr<Node>>* nodesList;
    Chain* chain;

    void startReceive() {
        Message pre;

        while(true) {
            Message request;

            socket->receive(boost::asio::buffer(&request, sizeof(Message)));

            if (std::memcmp(&pre, &request, sizeof(Message)) == 0)
                continue;
            if (request.type == Message::NANE)
                continue;

            std::memcpy(&pre, &request, sizeof(Message));

            //TODO
            Message response;
            boost::property_tree::ptree response_tree;

            if (request.type == Message::PEER_LIST_REQUEST) {
                for (size_t i=0; i<nodesList->size(); i++) {
                    if ((*nodesList)[i]->getIPAddress() == socket->remote_endpoint().address().to_string())
                        continue;

                    std::string position("hosts.ip");
                    position += i;

                    response_tree.put(position, (*nodesList)[i]->getIPAddress());
                }

                std::ostringstream oss;
                boost::property_tree::json_parser::write_json(oss, response_tree);

                std::memcpy(response.data, oss.str().data(), oss.str().size());

                socket->send(boost::asio::buffer(&response, sizeof(Message)));
            }

            else if (request.type == Message::BLOCK) {
                std::string block_text(request.data);

                Block block(block_text);

                if (chain->add(block))
                    std::cout << block_text << std::endl;
            }
        }
    }

public:
    Node(std::shared_ptr<boost::asio::ip::tcp::socket> socket) :
            socket(std::move(socket))
    {
        receive_thread = std::thread(&Node::startReceive, this);
        receive_thread.detach();
    }

    void setNodesList(std::vector<std::shared_ptr<Node>>* list) {
        nodesList = list;
    }

    void setChain(Chain* chain_) {
        chain = chain_;
    }

    bool isConnected() {
        return socket->is_open();
    }

    void disconnect() {
        socket->close();
    }

    std::string getIPAddress() {
        return socket->remote_endpoint().address().to_string();
    }

    int getPort() {
    }

//    std::vector<std::string>
    void nodesListRequest() {
        Message request;
        request.type = Message::PEER_LIST_REQUEST;

        socket->send(boost::asio::buffer(&request, sizeof(request)));

        Message response;
        socket->receive(boost::asio::buffer(&response, sizeof(Message)));

        std::cout << response.data << std::endl;
    }

    void send(Message message) {
        socket->send(boost::asio::buffer(&message, sizeof(Message)));
    }
};
