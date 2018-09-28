#include <mongols/ws_server.hpp>

int main(int, char**) {
    int port = 9090;
    const char* host = "127.0.0.1";
    mongols::ws_server server(host, port, 5000, 8096, 2/*0*/);

    auto f = [](const std::string& input
            , bool& keepalive
            , bool& send_to_other
            , mongols::tcp_server::client_t& client
            , mongols::tcp_server::filter_handler_function & send_to_other_filter) {
        keepalive = KEEPALIVE_CONNECTION;
        send_to_other = true;
        if (input == "close") {
            keepalive = CLOSE_CONNECTION;
            send_to_other = false;
        }
        return input;
    };
    server.run(f);
    //server.run();
}