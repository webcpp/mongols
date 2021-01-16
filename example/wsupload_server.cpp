#include <fstream>
#include <memory>
#include <mongols/util.hpp>
#include <mongols/ws_server.hpp>

int main(int, char **)
{
    int port = 9090;
    const char *host = "127.0.0.1";
    mongols::ws_server server(host, port, 5000, 8192, std::thread::hardware_concurrency() /*0*/);
    //    if (!server.set_openssl("openssl/localhost.crt", "openssl/localhost.key")) {
    //        return -1;
    //    }

    std::unordered_map<size_t, std::pair<std::string, std::shared_ptr<std::ofstream>>> file_manage;

    auto f = [&](const std::string &input, bool &keepalive, bool &send_to_other, mongols::tcp_server::client_t &client, mongols::tcp_server::filter_handler_function &send_to_other_filter, mongols::ws_server::ws_message_t &ws_msg_type) -> std::string {
        keepalive = KEEPALIVE_CONNECTION;
        send_to_other = false;
        if (ws_msg_type == mongols::ws_server::ws_message_t::BINARY)
        {
            *file_manage[client.sid].second << input;
            ws_msg_type = mongols::ws_server::ws_message_t::TEXT;
            return "continue";
        }
        std::vector<std::string> v = mongols::split(input, ':');
        if (v.size() > 1 && v[0] == "name")
        {
            file_manage[client.sid].first = v.back();
            file_manage[client.sid].second = std::make_shared<std::ofstream>("upload/" + file_manage[client.sid].first, std::ios::binary | std::ios::out | std::ios::ate);
            return "start upload";
        }
        if (input == "upload success")
        {
            file_manage.erase(client.sid);
        }
        return input;
    };
    //server.set_enable_origin_check(true);./
    //server.set_origin("http://localhost");
    //server.set_max_send_limit(5);
    server.set_shutdown([&]() {
        std::cout << "process " << getpid() << " exit.\n";
    });
    server.run(f);
    //server.run();
}