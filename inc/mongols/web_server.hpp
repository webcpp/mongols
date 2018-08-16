#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "http_server.hpp"

namespace mongols {

    class web_server {
    public:
        web_server() = delete;
        web_server(const std::string& host, int port
                , int timeout = 5000
                , size_t buffer_size = 1024
                , size_t thread_size = 0
                , size_t max_body_size = 1024
                , int max_event_size = 64);
        virtual~web_server();
        void set_root_path(const std::string&);
        void set_mime_type_file(const std::string&);
        void set_list_directory(bool);
        void run(const std::function<bool(const mongols::request&)>& req_filter);
    private:
        std::string root_path;
        std::unordered_map<std::string,std::string> mime_type;
        http_server *server;
        bool list_directory;
        std::string get_mime_type(const std::string&);
        void res_filter(const mongols::request&, mongols::response&);
        std::string create_list_directory_response(const std::string&);
       

    };
}

#endif /* WEB_SERVER_HPP */

