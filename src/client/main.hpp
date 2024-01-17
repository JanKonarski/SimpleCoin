#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/system/system_error.hpp>

#include <config.hpp>
#include <client/wallet.hpp>
#include <network/protocol.hpp>
#include <blockchain/transaction.hpp>

class Main {
private:
    Wallet wallet;
    std::string account_number;
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket;
    std::mutex socket_lock;
    std::vector<Transaction> history;
    std::mutex history_lock;

    std::string menu =
R"(

Select action:
 1) Show account balance
 2) Send transaction
 3) Show transaction history
>)";

    template<typename T1>
    struct triple {
        T1 first;
        T1 second;
        T1 third;

        triple(T1 first, T1 second, T1 third) : first(first), second(second), third(third) {}
    };

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
                socket.connect(boost::asio::ip::tcp::endpoint(address, CLIENT_PORT));
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

    void updateHistory() {
        Transaction empty(wallet.GetPublicKey(), 0, wallet.GetPublicKey());
        Message request(MessageType::TRANSACTION_REQUEST, empty.toJson());
        socket.send(boost::asio::buffer(&request, sizeof(Message)));

        Message response;
        socket.receive(boost::asio::buffer(&response, sizeof(Message)));

        if (response.type == MessageType::NULL_RESPONSE)
            return;

        history.clear();

        boost::property_tree::ptree ptree;
        std::istringstream is(response.data);
        boost::property_tree::json_parser::read_json(is, ptree);

        for (auto it : ptree.get_child("transactions")) {
            std::ostringstream os;
            boost::property_tree::json_parser::write_json(os, it.second);

            Transaction transaction(os.str());
            history.push_back(transaction);
        }
    }

    triple<float> accountBalance() {
        float money = 100.;
        float blocked = 0.;
        float expected = 0.;

        if (!history.empty())
            for (const auto& transaction : history) {
                if (transaction.input_pubkey == transaction.output_pubkey)
                    continue;

                if (transaction.input_pubkey == wallet.GetPublicKey()) {
                    if (transaction.status == false) {
                        blocked += transaction.amount;
                        money -= transaction.amount;
                    }
                    else
                        money -= transaction.amount;
                }
                else if (transaction.output_pubkey == wallet.GetPublicKey()) {
                    if (transaction.status == false)
                        expected += transaction.amount;
                    else
                        money += transaction.amount;
                }
            }

        return triple<float>(money, blocked, expected);
    }

    void sendTransaction() {
        std::string encode_receiver;
        std::cout << "Write receiver address: ";
        std::cin >> encode_receiver;

        if (account_number == encode_receiver) {
            std::cout << "You can't send money to yourself" << std::endl;
            return;
        }

        std::string receiver;
        CryptoPP::StringSource(encode_receiver, true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(receiver)
            )
        );

        float amount;
        std::cout << "Write amount: ";
        std::cin >> amount;

#ifdef CLIENT_VALIDATION
        if (accountBalance().first < amount) {
            std::cout << "You haven't got enough money" << std::endl;
            return;
        }
#endif

        Transaction transaction(wallet.GetPublicKey(), amount, receiver);
        transaction.sign(wallet.GetPrivateKey());

        boost::property_tree::ptree transaction_pt;
        std::istringstream is(transaction.toJson());
        boost::property_tree::json_parser::read_json(is, transaction_pt);

        boost::property_tree::ptree root_array_pt;
        boost::property_tree::ptree array_pt;
        array_pt.push_back(std::make_pair("", transaction_pt));
        root_array_pt.put_child("transactions", array_pt);

        std::ostringstream os;
        boost::property_tree::json_parser::write_json(os, root_array_pt);

        std::string transaction_json(os.str());

// DEBUG
std::cout << transaction_json << std::endl;

        Message message(MessageType::TRANSACTION_TRANSFER, transaction_json);

        {
            std::lock_guard<std::mutex> lock(socket_lock);
            socket.send(boost::asio::buffer(&message, sizeof(Message)));
        }

        {
            std::lock_guard<std::mutex> lock(history_lock);
            history.push_back(transaction);
        }
    }

    void transactionHistory() {
        std::cout << "History: " << std::endl;

        for (auto transaction : history)
            std::cout << transaction.toJson() << std::endl;
    }

public:
    Main(const std::vector<std::string> args)
    : socket(io_service)
    , wallet("../wordlist.txt")
    {
        arguments(args);

        wallet.SelectRandomWords(12);
        wallet.GenerateECDSAKeyPair();

        CryptoPP::StringSource(wallet.GetPublicKey(), true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(account_number),
                false
            )
        );

        while (true) {
            std::cout << menu;
            char action;
            std::cin >> action;

            if (action == '1') {
                updateHistory();
                std::cout << "Account state: " << account_number << std::endl;
                auto balance = accountBalance();
                std::cout << "Money -> " << balance.first << std::endl;
                std::cout << "Blocked -> " << balance.second << std::endl;
                std::cout << "Expected -> " << balance.third << std::endl;
            }

            if (action == '2')
                sendTransaction();

            if (action == '3')
                transactionHistory();
        }
    }
};