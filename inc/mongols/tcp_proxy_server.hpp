#ifndef TCP_PROXY_SERVER_HPP
#define TCP_PROXY_SERVER_HPP



#include <netinet/in.h>
#include <netdb.h> 

#include <ctime>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <functional>

#include "tcp_server.hpp"
#include "tcp_threading_server.hpp"
#include "lib/LRUCache11.hpp"
#include "request.hpp"

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
        void run(const tcp_server::filter_handler_function&, const std::function<bool(const mongols::request&)>&);

        void set_backend_server(const std::string&, int);

        void set_default_content(const std::string&);

        void set_default_http_content();
        void set_enable_http_mode(bool);
        void set_enable_http_lru_cache(bool);
        void set_http_lru_cache_size(size_t);
        void set_http_lru_cache_expires(long long);
        bool set_openssl(const std::string&, const std::string&);

    private:
        size_t index, backend_size, http_lru_cache_size;
        long long http_lru_cache_expires;
        bool enable_http, enable_http_lru_cache;
        tcp_server* server;
        std::vector<std::pair<std::string, int>> backend_server;
        std::unordered_map<size_t, std::shared_ptr<tcp_client>> clients;
        std::string default_content;
        lru11::Cache<std::string, std::shared_ptr<std::pair<std::string, time_t>>>* http_lru_cache;
        std::string work(const tcp_server::filter_handler_function&
                , const std::function<bool(const mongols::request&)>&
                , const std::pair<char*, size_t>&
                , bool&
                , bool&
                , tcp_server::client_t&
                , tcp_server::filter_handler_function&);

        static std::string DEFAULT_HTTP_CONTENT, DEFAULT_TCP_CONTENT;

    };
}

#endif /* TCP_PROXY_SERVER_HPP */

