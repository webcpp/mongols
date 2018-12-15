#ifndef SQLITE_SERVER_HPP
#define SQLITE_SERVER_HPP

#include <string>
#include "http_server.hpp"
#include "lib/sqlite/sqlite3pp.h"
#include "lib/sqlite/sqlite3ppext.h"


namespace mongols {

    class sqlite_server {
    public:
        sqlite_server() = delete;
        sqlite_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~sqlite_server();
        void set_lru_cache_expires(long long);
        void set_enable_lru_cache(bool);
        void set_lru_cache_size(size_t);
        void set_uri_rewrite(const std::pair<std::string, std::string>&);
        void run(const std::string& db_name);
    private:
        http_server *server;
        sqlite3pp::database *db;
    protected:
        virtual void work(const mongols::request& req, mongols::response& res);
        virtual bool filter(const mongols::request& req);
    };
}

#endif /* SQLITE_SERVER_HPP */

