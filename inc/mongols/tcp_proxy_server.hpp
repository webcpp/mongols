#ifndef TCP_PROXY_SERVER_HPP
#define TCP_PROXY_SERVER_HPP



#include <netinet/in.h>
#include <netdb.h> 


#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <functional>

#include "tcp_server.hpp"

namespace mongols {

    class tcp_client {
    public:

        tcp_client(const std::string& host = "127.0.0.1", int port = 8080);

        virtual~tcp_client();

        bool ok();

        ssize_t send(const char* str, size_t len);

        ssize_t recv(char* buffer, size_t len);


    private:

        void init();

    private:
        std::string host;
        int port, socket_fd;
        struct sockaddr_in server_addr;
        struct hostent *server;
    };

    class tcp_proxy_server {
    public:
        tcp_proxy_server() = delete;

        tcp_proxy_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8192
                , size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);
        virtual~tcp_proxy_server();

        void run(const tcp_server::filter_handler_function&);

        void set_back_server(const std::string&, int);

        void set_default_content(const std::string&);

    private:
        size_t index, back_end_size;
        tcp_server* server;
        std::vector<std::pair<std::string, int>> back_server;
        std::unordered_map<size_t, std::shared_ptr<tcp_client>> clients;
        std::string default_content;
        std::string work(const tcp_server::filter_handler_function&
                , const std::pair<char*, size_t>&
                , bool&
                , bool&
                , tcp_server::client_t&
                , tcp_server::filter_handler_function&);


    };
}

#endif /* TCP_PROXY_SERVER_HPP */

