#pragma once

#include "http_server.hpp"
#include "lib/sqlite/sqlite3pp.h"
#include "lib/sqlite/sqlite3ppext.h"
#include <string>

namespace mongols
{

    class sqlite_server
    {
    public:
        sqlite_server() = delete;
        sqlite_server(const std::string &host, int port, int timeout = 5000, size_t buffer_size = 8092, size_t thread_size = std::thread::hardware_concurrency(), size_t max_body_size = 4096, int max_event_size = 64);
        virtual ~sqlite_server();
        void set_lru_cache_expires(long long);
        void set_enable_lru_cache(bool);
        void set_lru_cache_size(size_t);
        void set_uri_rewrite(const std::pair<std::string, std::string> &);
        bool set_openssl(const std::string &, const std::string &, openssl::version_t = openssl::version_t::TLSv12, const std::string &ciphers = openssl::ciphers, long flags = openssl::flags);
        void set_enable_blacklist(bool);
        void set_enable_security_check(bool);
        void set_enable_whitelist(bool);
        void set_whitelist(const std::string &);
        void del_whitelist(const std::string &);
        void set_whitelist_file(const std::string &);
        void set_shutdown(const tcp_server::shutdown_function &);
        void run(const std::string &db_name);

    private:
        http_server *server;
        sqlite3pp::database *db;

    protected:
        virtual void work(const mongols::request &req, mongols::response &res);
        virtual bool filter(const mongols::request &req);
    };
} // namespace mongols
