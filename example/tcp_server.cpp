
#include <mongols/tcp_server.hpp>
#include <mongols/tcp_threading_server.hpp>
#include <unistd.h>

int main(int, char **)
{
    auto f = [](const std::pair<char *, size_t> &input, bool &keepalive, bool &send_to_other, mongols::tcp_server::client_t &client, mongols::tcp_server::filter_handler_function &send_to_other_filter) {
        keepalive = KEEPALIVE_CONNECTION;
        send_to_other = true;
        return std::string(input.first, input.second);
    };
    int port = 9090;
    const char *host = "127.0.0.1";

    mongols::tcp_server
        //    mongols::tcp_threading_server
        server(host, port);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }
    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.set_whitelist_file("etc/whitelist.txt");
    server.set_enable_whitelist(true);
    server.run(f);
}