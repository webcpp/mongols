#pragma once

#include "request.hpp"
#include "tcp_server.hpp"
#include "tcp_threading_server.hpp"

namespace mongols
{

    class ws_server
    {
    public:
        enum ws_message_t
        {
            TEXT = 0,
            BINARY = 1
        };
        typedef tcp_server::handler_function handler_function;
        typedef std::function<std::string(
            const std::string &, bool &, bool &, tcp_server::client_t &, tcp_server::filter_handler_function &, ws_message_t &)>
            message_handler_function;

    public:
        ws_server() = delete;
        virtual ~ws_server();
        ws_server(const std::string &host, int port, int timeout = 5000, size_t buffer_size = 8192,
                  size_t thread_size = std::thread::hardware_concurrency(), int max_event_size = 64);

    public:
        void run(const message_handler_function &);
        void run();
        bool set_openssl(const std::string &, const std::string &, openssl::version_t = openssl::version_t::TLSv12, const std::string &ciphers = openssl::ciphers, long flags = openssl::flags);
        void set_origin(const std::string &);
        void set_enable_origin_check(bool);
        void set_enable_blacklist(bool);
        void set_enable_whitelist(bool);
        void set_whitelist(const std::string &);
        void del_whitelist(const std::string &);
        void set_whitelist_file(const std::string &);
        void set_enable_security_check(bool);
        void set_shutdown(const tcp_server::shutdown_function &);

    private:
        virtual std::string work(const message_handler_function &, const std::pair<char *, size_t> &, bool &, bool &, tcp_server::client_t &, tcp_server::filter_handler_function &);
        std::string ws_json_parse(const std::string &message, bool &keepalive, bool &send_to_other, tcp_server::client_t &client, tcp_server::filter_handler_function &send_to_other_filter, ws_message_t &ws_msg_type);
        bool ws_handshake(const mongols::request &req, std::string &response);
        int ws_parse(const std::pair<char *, size_t> &frame, std::string &message, bool &is_final);

    private:
        mongols::tcp_server *server;
        bool enable_origin_check;
        std::string origin;
        std::unordered_map<size_t, std::string> message_buffer;
    };
} // namespace mongols
