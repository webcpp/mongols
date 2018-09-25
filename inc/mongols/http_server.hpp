#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP



#include "tcp_server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "http_request_parser.hpp"
#include "lib/leveldb/db.h"
#include "lib/leveldb/options.h"



namespace mongols {

    class http_server {
    public:
        http_server() = delete;
        http_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8192
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~http_server();

    public:
        void run(const std::function<bool(const mongols::request&)>& req_filter
                , const std::function<void(const mongols::request&, mongols::response&)>& res_filter);

        void set_session_expires(long long);
        void set_cache_expires(long long);
        void set_enable_session(bool);
        void set_enable_cache(bool);
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
        void set_cache_size(size_t);
        void set_enable_compression(bool);
        void set_db_path(const std::string&);
    private:
        std::string work(
                const std::function<bool(const mongols::request&)>& req_filter
                , const std::function<void(const mongols::request& req, mongols::response&)>& res_filter
                , const std::pair<char*, size_t>&
                , bool&
                , bool&
                , tcp_server::client_t&
                , tcp_server::filter_handler_function&);
    private:
        std::string create_response(mongols::response& res, bool b);
        std::string get_status_text(int status);
        std::string tolower(std::string&);
        void upload(mongols::request&, const std::string&);
        std::string serialize(const std::unordered_map<std::string, std::string>&);
        void deserialize(const std::string&, std::unordered_map<std::string, std::string>&);
    private:
        mongols::tcp_server *server;
        size_t max_body_size;
        leveldb::DB *db;
        leveldb::Options db_options;
        long long session_expires, cache_expires;
        bool enable_session, enable_cache;
        std::string db_path;


    };


}

#endif /* HTTP_SERVER_HPP */

