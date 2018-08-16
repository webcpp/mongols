#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <string>

#include "request.hpp"
#include "lib/http_parser.h"

namespace mongols{
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
}

#endif /* HTTP_REQUEST_PARSER_HPP */

