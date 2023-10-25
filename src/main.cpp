#include <mutex>

std::mutex mutex;

#include <main.hpp>

int main(int argc, char **argv) {

    std::vector<std::string> args(argv, argv+argc);
    Main main(args);

    return EXIT_SUCCESS;
}
