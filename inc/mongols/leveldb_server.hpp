#ifndef LEVELDB_SERVER_HPP
#define LEVELDB_SERVER_HPP

#include <netdb.h>

#include "http_server.hpp"
#include "lib/leveldb/db.h"
#include "lib/leveldb/options.h"

namespace mongols {

    class leveldb_server {
    public:
        leveldb_server() = delete;
        leveldb_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~leveldb_server();
        void set_max_open_files(int);
        void set_write_buffer_size(size_t);
        void set_max_file_size(size_t);
        void set_cache_size(size_t);
        void set_enable_compression(bool);
        void set_enable_lru_cache(bool);
        void set_lru_cache_size(size_t);
        void set_lru_cache_expires(long long);
        void set_uri_rewrite(const std::pair<std::string, std::string>&);
        void run(const std::string&);
    private:
        http_server* server;
        leveldb::DB *db;
        leveldb::Options options;
    protected:
        virtual void work(const mongols::request& req, mongols::response& res);
        virtual bool filter(const mongols::request& req);

    };
}


#endif /* LEVELDB_SERVER_HPP */

