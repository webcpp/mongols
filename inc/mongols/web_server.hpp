#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <unordered_map>
#include <utility>
#include <sys/stat.h>
#include "http_server.hpp"


namespace mongols {

    class web_server {
    public:
        web_server() = delete;
        web_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 8092
                , size_t thread_size = std::thread::hardware_concurrency()
                , size_t max_body_size = 4096
                , int max_event_size = 64);
        virtual~web_server();
        void set_root_path(const std::string&);
        void set_mime_type_file(const std::string&);
        void set_list_directory(bool);
        void set_enable_mmap(bool);
        void set_cache_expires(long long);
        void run(const std::function<bool(const mongols::request&)>& req_filter);
    private:
        long long cache_expires;
        std::string root_path;
        std::unordered_map<std::string, std::string> mime_type;
        std::unordered_map<std::string, std::pair<char*, struct stat>> file_mmap;
        http_server *server;
        bool list_directory, enable_mmap;
        std::string get_mime_type(const std::string&);
        void res_filter(const mongols::request&, mongols::response&);
        std::string create_list_directory_response(const std::string&);
        void res_filter_with_mmap(const mongols::request&, mongols::response&);


    };
}

#endif /* WEB_SERVER_HPP */

