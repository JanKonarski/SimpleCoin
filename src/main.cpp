#include <thread>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <config.hpp>
#include <server.hpp>

int main(int argc, char** argv) {
    try {
        boost::asio::io_service io_service;
        std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> connections;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                ("help", "produce help message")
                ("node", boost::program_options::value<std::vector<std::string>>()->multitoken(),
                        "node addresses");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc,
                                                                                 argv,
                                                                                 desc),
                                      vm);
        boost::program_options::notify(vm);

        if (!vm["node"].empty()) {
            for (auto address: vm["node"].as<std::vector<std::string>>()) {
                boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address),
                                                        PORT);
                boost::system::error_code error;
                std::shared_ptr<boost::asio::ip::tcp::socket> socket(new boost::asio::ip::tcp::socket(io_service));
                socket->connect(endpoint, error);

                if (error)
                    std::cout << "No connection with " << address << std::endl;
                else
                    connections.push_back(socket);
            }
        }

        Server server(io_service, connections);

        std::string line;
        while (std::getline(std::cin, line))
            server.send(line);
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Other exception" << std::endl;
    }

    return EXIT_SUCCESS;
}
