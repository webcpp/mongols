
#include <iostream>
#include <mongols/udp_server.hpp>

int main(int, char **)
{
    auto f = [](const std::pair<char *, size_t> &input, const mongols::udp_server::client_t &client) {
        return std::string(input.first, input.second);
    };
    int port = 9090;
    const char *host = "127.0.0.1";

    mongols::udp_server::setsockopt_cb = [&](int fd) {
        // call setsockopt
    };

    mongols::udp_server server(host, port);

    server.set_shutdown([&](int fd) {
        // call setsockopt
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.set_whitelist_file("etc/whitelist.txt");
    server.set_enable_whitelist(true);
    server.run(f);
}