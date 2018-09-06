#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <string>

#include "request.hpp"
#include "lib/http_parser.h"

namespace mongols {

    class http_request_parser {
    public:
        http_request_parser() = delete;
        http_request_parser(mongols::request& req);
        virtual~http_request_parser() = default;

        bool parse(const std::string& str);
        bool parse(const char*,size_t);


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
    private:
        static int on_message_begin(http_parser * p);
        static int on_message_complete(http_parser* p);
        static int on_header_field(http_parser *p, const char *buf, size_t len);
        static int on_header_value(http_parser *p, const char *buf, size_t len);
        static int on_url(http_parser *p, const char *buf, size_t len);
        static int on_status(http_parser*p, const char *at, size_t length);
        static int on_body(http_parser *p, const char *buf, size_t len);
        static int on_headers_complete(http_parser * p);
        static int on_chunk_header(http_parser* p);
        static int on_chunk_complete(http_parser* p);

    };
}

#endif /* HTTP_REQUEST_PARSER_HPP */

