#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP



#include "tcp_server.hpp"
#include "servlet.hpp"
#include "lib/http_parser.h"
#include "lib/lrucache.hpp"
#include "lib/redis.hpp"


namespace mongols {

    class http_request_parser {
    public:
        http_request_parser() = delete;
        http_request_parser(mongols::request& req);
        virtual~http_request_parser() = default;

        bool parse(const std::string& str);


        const std::string& get_body()const;
        std::string& get_body();

    private:

        struct tmp_ {
            std::pair<std::string, std::string> pair;
            http_request_parser* parser;
        };
    private:
        tmp_ tmp;
        struct http_parser parser;
        struct http_parser_settings settings;
        mongols::request& req;
        std::string body;


    };

    class http_server {
    public:

        class cache_t {
        public:
            cache_t();
            cache_t(const std::string&, long long);
            virtual~cache_t() = default;
            bool expired()const;
            const std::string& get()const;
            cache_t& set_data(const std::string&);
            cache_t& set_expires(long long);
        private:
            std::string data;
            time_t t;
            long long expires;
        };
    public:
        http_server() = delete;
        http_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 1024
                , size_t thread_size = 0
                , size_t max_body_size = 1024
                , int max_event_size = 64);
        virtual~http_server();

    public:
        void run(const std::function<bool(const mongols::request&)>& req_filter
                , const std::function<void(const mongols::request&, mongols::response&)>& res_filter);

        void set_session_expires(long long);
        void set_cache_expires(long long);
        void set_enable_session(bool);
        void set_enable_cache(bool);
    private:
        std::string work(
                const std::function<bool(const mongols::request&)>& req_filter
                , const std::function<void(const mongols::request& req, mongols::response&)>& res_filter
                , const std::string&
                , bool&
                , bool&
                , std::pair<size_t, size_t>&
                , tcp_server::filter_handler_function&);
    private:
        bool parse_reqeust(const std::string& str, mongols::request& req, std::string& body);
        std::string create_response(mongols::response& res, bool b);
        std::string get_status_text(int status);
        std::string tolower(std::string&);
        void upload(mongols::request&, const std::string&);
    private:
        mongols::tcp_server *server;
        size_t max_body_size;
        mongols::redis redis;
        long long session_expires, cache_expores;
        bool enable_session,enable_cache;


    };


}

#endif /* HTTP_SERVER_HPP */

