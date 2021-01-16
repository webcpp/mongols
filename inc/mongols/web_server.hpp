#pragma once

#include "http_server.hpp"
#include <sys/stat.h>
#include <unordered_map>
#include <utility>

namespace mongols
{

    class web_server
    {
    public:
        web_server() = delete;
        web_server(const std::string &host, int port, int timeout = 5000, size_t buffer_size = 8192, size_t thread_size = std::thread::hardware_concurrency(), size_t max_body_size = 4096, int max_event_size = 64);
        virtual ~web_server();
        void set_root_path(const std::string &);
        void set_mime_type_file(const std::string &);
        void set_list_directory(bool);
        void set_enable_mmap(bool);
        void set_lru_cache_expires(long long);
        void set_enable_lru_cache(bool);
        void set_lru_cache_size(size_t);
        void set_uri_rewrite(const std::pair<std::string, std::string> &);
        bool set_openssl(const std::string &, const std::string &, openssl::version_t = openssl::version_t::TLSv12, const std::string &ciphers = openssl::ciphers, long flags = openssl::flags);
        void set_enable_blacklist(bool);
        void set_enable_whitelist(bool);
        void set_whitelist(const std::string &);
        void del_whitelist(const std::string &);
        void set_whitelist_file(const std::string &);
        void set_enable_security_check(bool);
        void set_shutdown(const tcp_server::shutdown_function &);
        void run(const std::function<bool(const mongols::request &)> &req_filter);

    private:
        static std::string dir_index_template;
        std::string root_path;
        std::unordered_map<std::string, std::string> mime_type;
        std::unordered_map<std::string, std::pair<char *, struct stat>> file_mmap;
        http_server *server;
        bool list_directory, enable_mmap;
        std::string get_mime_type(const std::string &);
        void res_filter(const mongols::request &, mongols::response &);
        std::string create_list_directory_response(const std::string &);
        void res_filter_with_mmap(const mongols::request &, mongols::response &);
    };
} // namespace mongols
