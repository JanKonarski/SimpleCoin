#pragma once

#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <boost/program_options.hpp>
#include <boost/system/system_error.hpp>

#include <config.hpp>
#include <client/wallet.hpp>
#include <network/protocol.hpp>
#include <blockchain/block.hpp>
#include <blockchain/chain.hpp>
#include <blockchain/merkle.hpp>
#include <blockchain/transaction.hpp>

class Main {
private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket sender;
    boost::asio::ip::tcp::socket receiver;
    std::thread io_service_thread;

    void arguments(std::vector<std::string> args) {
        std::vector<const char*> char_args;
        std::transform(args.begin(), args.end(), std::back_inserter(char_args),
                       [](const std::string& str){return str.c_str();});

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("node", boost::program_options::value<std::string>(),
                 "node address");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(char_args.size(),
                                                                                 char_args.data(),
                                                                                 desc),
                                      vm);
        boost::program_options::notify(vm);

        if (vm.count("help") | vm["node"].empty()) {
            std::ostringstream oss;
            desc.print(oss);
            std::cout.write(oss.str().c_str(), oss.str().size());

            exit(EXIT_SUCCESS);
        }

        if (!vm["node"].empty()) {
            try {
                auto address = boost::asio::ip::address_v4::from_string((vm["node"].as<std::string>()));
                sender.connect(boost::asio::ip::tcp::endpoint(address, MINER_PORT));

                boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(
                        boost::asio::ip::tcp::v4(),
                        MINER_PORT + 1
                ));

                acceptor.accept(receiver);
            }
            catch (const std::exception& e) {
//                std::string msg("Arguent address error\n");
                std::string msg("Exception: ");
                msg += e.what();
                msg += "\n";
                std::cerr.write(msg.c_str(), msg.size());

                exit(EXIT_FAILURE);
            }
        }
    }

public:
    Main(const std::vector<std::string> args)
    : sender(io_service),
      receiver(io_service)
    {
        io_service_thread = std::thread(boost::bind(&boost::asio::io_service::run,&io_service));
        io_service_thread.detach();

        arguments(args);

        std::string encode_pubkey = "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEoldRXz4vggg7wQOut4dmxwj1Xux2boVKljQyGKx8kfGxODwLGdtk/mIrfo5dt8trHIgofU9DAO+O5Hm4T2IhNg==";
        std::string pubkey;
        CryptoPP::StringSource(encode_pubkey, true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(pubkey)
            )
        );

        // TODO
        Chain chain;
        for (int i=0; i<5; i++) {
            Wallet wallet("../wordlist.txt");
            wallet.SelectRandomWords(12);
            wallet.GenerateECDSAKeyPair();

            Transaction transaction(wallet.GetPublicKey(), 1, pubkey);
            transaction.sign(wallet.GetPrivateKey());

            pubkey = wallet.GetPublicKey();

            Merkle merkle(std::vector<Transaction>({transaction}));

            Block block(merkle, chain.getLast());
            block.mineBlock();

            chain.add(block);
        }

        std::cout << chain.toJson() << std::endl;

        while (true) {
            Message request;
            receiver.receive(boost::asio::buffer(&request, sizeof(Message)));

            if (request.type == MessageType::CHAIN_REQUEST) {
                Message message(MessageType::CHAIN_TRANSFER, chain.toJson());
                sender.send(boost::asio::buffer(&message, sizeof(Message)));
            }
        }
    }
};