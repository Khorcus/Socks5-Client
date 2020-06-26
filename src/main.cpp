#include <ctime>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include "socks/SocksClient.hpp"

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 9) {
        std::cerr
                << "Usage: Socks5-Client socks_host socks_port server_host server_port data_count session_count time thread_count"
                << std::endl;
        std::cerr << "Example: Socks5-Client 127.0.0.1 8080 127.0.0.1 8081 500 10 60 4" << std::endl;
        exit(EXIT_FAILURE);
    }

    char *socks_host = argv[1];
    uint16_t socks_port = htons(std::atoi(argv[2]));
    char *server_host = argv[3];
    uint16_t server_port = htons(std::atoi(argv[4]));
    uint16_t data_count = std::atoi(argv[5]);
    std::string test_string = std::string(data_count, 'a');
    uint16_t session_count = std::atoi(argv[6]);
    uint16_t time = std::atoi(argv[7]) * 1000;
    uint16_t thread_count = std::atoi(argv[8]);
    unsigned long long k = 0;
    auto thread_pool = std::vector<std::thread>();
    thread_pool.reserve(thread_count);
    auto client_pool = std::vector<SocksClient>(thread_count);

    uint16_t sessions_per_thread = session_count / thread_count;
    uint16_t unallocated_sessions = session_count % thread_count;

    sessions_per_thread++;

    for (uint16_t i = 0; i < thread_count; ++i) {
        if (unallocated_sessions == 0) {
            sessions_per_thread--;
        }
        client_pool[i].init(socks_host, socks_port, server_host, server_port);
        thread_pool.emplace_back(std::thread(&SocksClient::start_test, &client_pool[i], sessions_per_thread, test_string, time));
    }
    for (uint16_t i = 0; i < thread_count; ++i) {
        thread_pool[i].join();
    }
    for (uint16_t i = 0; i < thread_count; ++i) {
        k += client_pool[i].get_ping_count();
    }
    std::cout << "Ping count: " << k << std::endl;

}
