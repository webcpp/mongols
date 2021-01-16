#pragma once

#include <netdb.h>
#include <netinet/in.h>

#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lib/LRUCache11.hpp"
#include "request.hpp"
#include "tcp_server.hpp"
#include "tcp_threading_server.hpp"

namespace mongols
{

    class tcp_client
    {
    public:
        tcp_client(const std::string &host = "127.0.0.1", int port = 8080, bool enable_openssl = false);

        virtual ~tcp_client();

        bool ok();

        ssize_t send(const char *str, size_t len);

        ssize_t recv(char *buffer, size_t len);

    private:
        void init();

    private:
        std::string host;
        int port, socket_fd;
        struct sockaddr_in server_addr;
        struct hostent *server;
        bool enable_openssl;
        SSL *ssl;

    private:
        class ctx_t
        {
        public:
            ctx_t();
            virtual ~ctx_t();
            SSL_CTX *get();

        private:
            SSL_CTX *ctx;
        };
        static ctx_t ctx;
        static const int ssl_session_ctx_id;
    };

    class tcp_proxy_server
    {
    public:
        tcp_proxy_server() = delete;

        tcp_proxy_server(const std::string &host, int port, int timeout = 5000, size_t buffer_size = 8192, size_t thread_size = std::thread::hardware_concurrency(), int max_event_size = 64);
        virtual ~tcp_proxy_server();

        void run(const tcp_server::filter_handler_function &);
        void run(const tcp_server::filter_handler_function &, const std::function<bool(const mongols::request &)> &);

        void set_backend_server(const std::string &, int, bool = false);

        void set_default_content(const std::string &);
        void set_enable_tcp_send_to_other(bool);

        void set_default_http_content();
        void set_enable_http_lru_cache(bool);
        void set_http_lru_cache_size(size_t);
        void set_http_lru_cache_expires(long long);
        bool set_openssl(const std::string &, const std::string &, openssl::version_t = openssl::version_t::TLSv12, const std::string &ciphers = openssl::ciphers, long flags = openssl::flags);
        void set_enable_blacklist(bool);
        void set_enable_whitelist(bool);
        void set_whitelist(const std::string &);
        void del_whitelist(const std::string &);
        void set_whitelist_file(const std::string &);
        void set_enable_security_check(bool);
        void set_shutdown(const tcp_server::shutdown_function &);

    private:
        class backend_server_t
        {
        public:
            backend_server_t() = delete;
            backend_server_t(const std::string &, int port, bool);
            virtual ~backend_server_t() = default;

        public:
            std::string server;
            int port;
            bool enable_ssl;
        };

    private:
        size_t index, backend_size, http_lru_cache_size;
        long long http_lru_cache_expires;
        bool enable_http_lru_cache, enable_tcp_send_to_other;
        tcp_server *server;
        std::vector<backend_server_t> backend_server;
        std::unordered_map<size_t, std::shared_ptr<tcp_client>> clients;
        std::string default_content;
        lru11::Cache<std::string, std::shared_ptr<std::pair<std::string, time_t>>> *http_lru_cache;
        std::string work(const tcp_server::filter_handler_function &, const std::pair<char *, size_t> &, bool &, bool &, tcp_server::client_t &, tcp_server::filter_handler_function &);
        std::string http_work(const tcp_server::filter_handler_function &, const std::function<bool(const mongols::request &)> &, const std::pair<char *, size_t> &, bool &, bool &, tcp_server::client_t &, tcp_server::filter_handler_function &);

        static std::string DEFAULT_HTTP_CONTENT, DEFAULT_TCP_CONTENT;
    };
} // namespace mongols
