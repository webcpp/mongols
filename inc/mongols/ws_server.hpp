#ifndef WS_SERVER_HPP
#define WS_SERVER_HPP

#include "tcp_server.hpp"
#include "tcp_threading_server.hpp"



namespace mongols {

    class ws_server {
    public:
        typedef tcp_server::handler_function handler_function;
        typedef std::function<std::string(
                const std::string&
                , bool&
                , bool&
                , tcp_server::client_t&
                , tcp_server::filter_handler_function&) > message_handler_function;
    public:
        ws_server() = delete;
        virtual~ws_server();
        ws_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092,
                size_t thread_size = std::thread::hardware_concurrency()
                , int max_event_size = 64);

    public:
        void run(const message_handler_function&);
        void run();
        bool set_openssl(const std::string&, const std::string&
                , openssl::version_t = openssl::version_t::TLSv12
                , const std::string& ciphers = openssl::ciphers
                , long flags = openssl::flags);
        void set_origin(const std::string&);
        void set_max_send_limit(size_t);
        void set_enable_origin_check(bool);
        void set_enable_blacklist(bool);
    private:
        virtual std::string work(const message_handler_function&
                , const std::pair<char*, size_t>&
                , bool&
                , bool&
                , tcp_server::client_t&
                , tcp_server::filter_handler_function&);
        std::string ws_json_parse(const std::string& message
                , bool& keepalive
                , bool& send_to_other
                , tcp_server::client_t& client
                , tcp_server::filter_handler_function& send_to_other_filter);
        bool ws_handshake(const std::pair<char*, size_t> &request, std::string &response, std::unordered_map<std::string, std::string>& headers);
        int ws_parse(const std::pair<char*, size_t>& frame, std::string& message);

    private:
        mongols::tcp_server *server;
        bool enable_origin_check;
        size_t max_send_limit;
        std::string origin;

    };
}

#endif /* WS_SERVER_HPP */

