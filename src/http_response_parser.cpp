#include "http_response_parser.hpp"

namespace mongols
{

    int http_response_parser::on_message_begin(http_parser *p)
    {
        return 0;
    }

    int http_response_parser::on_message_complete(http_parser *p)
    {
        return 0;
    }

    int http_response_parser::on_body(http_parser *p, const char *buf, size_t len)
    {
        http_response_parser::tmp_ *THIS = (http_response_parser::tmp_ *)p->data;
        THIS->parser->body.assign(buf, len);
        return 0;
    }

    int http_response_parser::on_chunk_complete(http_parser *p)
    {
        return 0;
    }

    int http_response_parser::on_chunk_header(http_parser *p)
    {
        return 0;
    }

    int http_response_parser::on_header_field(http_parser *p, const char *buf, size_t len)
    {
        http_response_parser::tmp_ *THIS = (http_response_parser::tmp_ *)p->data;
        THIS->pair.first = std::move(std::string(buf, len));
        THIS->parser->res.headers.insert(std::move(std::make_pair(THIS->pair.first, "")));
        return 0;
    }

    int http_response_parser::on_header_value(http_parser *p, const char *buf, size_t len)
    {
        http_response_parser::tmp_ *THIS = (http_response_parser::tmp_ *)p->data;
        THIS->parser->res.headers.find(THIS->pair.first)->second = std::move(std::string(buf, len));
        return 0;
    }

    int http_response_parser::on_headers_complete(http_parser *p)
    {
        return 0;
    }

    int http_response_parser::on_status(http_parser *p, const char *at, size_t length)
    {
        http_response_parser::tmp_ *THIS = (http_response_parser::tmp_ *)p->data;
        THIS->parser->res.status = p->status_code;
        return 0;
    }

    int http_response_parser::on_url(http_parser *p, const char *buf, size_t len)
    {
        return 0;
    }

    http_response_parser::http_response_parser(mongols::response &res)
        : tmp(), parser(), settings(), res(res), body()
    {
        res.headers.erase("Content-Type");
        http_parser_init(&this->parser, HTTP_RESPONSE);
        http_parser_settings_init(&this->settings);
        this->tmp.parser = this;
        this->parser.data = &this->tmp;

        this->settings.on_message_begin = http_response_parser::on_message_begin;

        this->settings.on_header_field = http_response_parser::on_header_field;

        this->settings.on_header_value = http_response_parser::on_header_value;

        this->settings.on_url = http_response_parser::on_url;

        this->settings.on_status = http_response_parser::on_status;

        this->settings.on_body = http_response_parser::on_body;

        this->settings.on_headers_complete = http_response_parser::on_headers_complete;

        this->settings.on_message_complete = http_response_parser::on_message_complete;

        this->settings.on_chunk_header = http_response_parser::on_chunk_header;

        this->settings.on_chunk_complete = http_response_parser::on_chunk_complete;
    }

    bool http_response_parser::parse(const std::string &str)
    {
        return http_parser_execute(&this->parser, &this->settings, str.c_str(), str.size()) == str.size();
    }

    bool http_response_parser::parse(const char *str, size_t len)
    {
        return http_parser_execute(&this->parser, &this->settings, str, len) == len;
    }

    const std::string &http_response_parser::get_body() const
    {
        return this->body;
    }

    std::string &http_response_parser::get_body()
    {
        return this->body;
    }
} // namespace mongols