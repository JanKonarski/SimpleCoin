#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <iostream>
#include <exception>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <p2p.hpp>
#include <peer.hpp>

extern std::mutex mutex;

class Main {
private:
    std::shared_ptr<boost::asio::io_service> ioService;
    std::shared_ptr<boost::asio::io_service::work> work;
    std::shared_ptr<std::vector<std::shared_ptr<Peer>>> nodeList;
    std::shared_ptr<P2P> p2p;

    std::vector<boost::asio::ip::address>
    arguments(const std::vector<std::string>& args) {
        std::vector<const char*> charArgs;
        std::transform(args.begin(), args.end(), std::back_inserter(charArgs),
                       [](const std::string& str){return str.c_str();});

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("node", boost::program_options::value<std::vector<std::string>>()->multitoken(),
                 "node addresses");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(charArgs.size(),
                                                                                 charArgs.data(),
                                                                                 desc),
                                      vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(EXIT_SUCCESS);
        }

        std::vector<boost::asio::ip::address> addresses;
        if (!vm["node"].empty())
            for (auto strAddress: vm["node"].as<std::vector<std::string>>()) {
                try {
                    auto address = boost::asio::ip::address::from_string(strAddress);
                    addresses.push_back(address);
                }
                catch (...) {
                    std::cerr << "Value " << strAddress << " is not network address" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

        return addresses;
    }

public:
    Main(const std::vector<std::string> args) {
        try {
            ioService = std::make_shared<boost::asio::io_service>();
            work = std::make_shared<boost::asio::io_service::work>(*ioService);

            nodeList = std::make_shared<std::vector<std::shared_ptr<Peer>>>();

            p2p = std::make_shared<P2P>(*ioService);
            p2p->setNodeList(nodeList);

            auto addresses = arguments(args);
            if (!addresses.empty())
                p2p->makeConnection(addresses);

            p2p->startListening();

//            if (args.size() > 1) {
//                auto remoteNodeList = (*nodeList)[0]->getNodeList();
//                std::cout << "receive node list: " << remoteNodeList.size() << std::endl;
//            }

            std::thread ioServiceThread(boost::bind(&boost::asio::io_service::run, ioService));
            ioServiceThread.detach();

            std::string line;
            while (std::getline(std::cin, line)) {
                std::cout << "***    " << line << std::endl;

                for (auto node : *nodeList) {
                    std::string msg = "{\"ACTION\": \"show\",\n";
                    msg += "\"text\": \"" + line + "\"}";
                    node->send(msg);
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        catch (...) {
            std::cerr << "Other exception" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

};
