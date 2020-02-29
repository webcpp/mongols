#ifndef HTTP_RESPONSE_PARSER_HPP
#define HTTP_RESPONSE_PARSER_HPP

#include <string>

#include "lib/llhttp.h"
#include "response.hpp"

namespace mongols {

class http_response_parser {
public:
    http_response_parser() = delete;
    http_response_parser(mongols::response& res);
    virtual ~http_response_parser() = default;

    bool parse(const std::string& str);
    bool parse(const char*, size_t);

    const std::string& get_body() const;
    std::string& get_body();

private:
    struct tmp_ {
        std::pair<std::string, std::string> pair;
        http_response_parser* parser;
    };

private:
    tmp_ tmp;
    llhttp_t parser;
    llhttp_settings_t settings;
    mongols::response& res;
    std::string body;

private:
    static int on_message_begin(llhttp_t* p);
    static int on_message_complete(llhttp_t* p);
    static int on_header_field(llhttp_t* p, const char* buf, size_t len);
    static int on_header_value(llhttp_t* p, const char* buf, size_t len);
    static int on_url(llhttp_t* p, const char* buf, size_t len);
    static int on_status(llhttp_t* p, const char* at, size_t length);
    static int on_body(llhttp_t* p, const char* buf, size_t len);
    static int on_headers_complete(llhttp_t* p);
    static int on_chunk_header(llhttp_t* p);
    static int on_chunk_complete(llhttp_t* p);
};
}

#endif /* HTTP_RESPONSE_PARSER_HPP */
