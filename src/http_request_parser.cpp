
#include "http_request_parser.hpp"
#include "lib/url_parser.h"
#include <iostream>
namespace mongols {

int http_request_parser::on_message_begin(llhttp_t* p)
{
    return 0;
}

int http_request_parser::on_message_complete(llhttp_t* p)
{
    return 0;
}

int http_request_parser::on_body(llhttp_t* p, const char* buf, size_t len)
{
    http_request_parser::tmp_* THIS = (http_request_parser::tmp_*)p->data;
    THIS->parser->body.assign(buf, len);
    return 0;
}

int http_request_parser::on_chunk_complete(llhttp_t* p)
{
    return 0;
}

int http_request_parser::on_chunk_header(llhttp_t* p)
{
    return 0;
}

int http_request_parser::on_header_field(llhttp_t* p, const char* buf, size_t len)
{
    http_request_parser::tmp_* THIS = (http_request_parser::tmp_*)p->data;
    THIS->pair.first = std::move(std::string(buf, len));
    THIS->parser->req.headers.insert(std::move(std::make_pair(THIS->pair.first, "")));
    return 0;
}

int http_request_parser::on_header_value(llhttp_t* p, const char* buf, size_t len)
{
    http_request_parser::tmp_* THIS = (http_request_parser::tmp_*)p->data;
    THIS->parser->req.headers[THIS->pair.first] = std::move(std::string(buf, len));

    return 0;
}

int http_request_parser::on_headers_complete(llhttp_t* p)
{
    return 0;
}

int http_request_parser::on_status(llhttp_t* p, const char* at, size_t length)
{
    return 0;
}

int http_request_parser::on_url(llhttp_t* p, const char* buf, size_t len)
{
    http_request_parser::tmp_* THIS = (http_request_parser::tmp_*)p->data;
    THIS->parser->req.method = llhttp_method_name((llhttp_method_t)p->method);
    struct http_parser_url u;
    http_parser_url_init(&u);
    http_parser_parse_url(buf, len, 0, &u);
    if (u.field_set & (1 << UF_PATH)) {
        THIS->parser->req.uri.assign(buf + u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
    }
    if (u.field_set & (1 << UF_QUERY)) {
        THIS->parser->req.param.assign(buf + u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
    }

    return 0;
}

http_request_parser::http_request_parser(mongols::request& req)
    : tmp()
    , parser()
    , settings()
    , req(req)
    , body()
{
    llhttp_settings_init(&this->settings);
    this->settings.on_message_begin = http_request_parser::on_message_begin;

    this->settings.on_header_field = http_request_parser::on_header_field;

    this->settings.on_header_value = http_request_parser::on_header_value;

    this->settings.on_url = http_request_parser::on_url;

    this->settings.on_status = http_request_parser::on_status;

    this->settings.on_body = http_request_parser::on_body;

    this->settings.on_headers_complete = http_request_parser::on_headers_complete;

    this->settings.on_message_complete = http_request_parser::on_message_complete;

    this->settings.on_chunk_header = http_request_parser::on_chunk_header;

    this->settings.on_chunk_complete = http_request_parser::on_chunk_complete;

    llhttp_init(&this->parser, HTTP_REQUEST, &this->settings);
    this->tmp.parser = this;
    this->parser.data = &this->tmp;
}

bool http_request_parser::parse(const std::string& str)
{
    return llhttp_execute(&this->parser, str.c_str(), str.size()) == HPE_OK;
}

bool http_request_parser::parse(const char* str, size_t len)
{
    return llhttp_execute(&this->parser, str, len) == HPE_OK;
}

const std::string& http_request_parser::get_body() const
{
    return this->body;
}

std::string& http_request_parser::get_body()
{
    return this->body;
}

bool http_request_parser::keep_alive() const
{
    return llhttp_should_keep_alive(&this->parser);
}

bool http_request_parser::upgrade() const
{
    return this->parser.upgrade == 1;
}
}