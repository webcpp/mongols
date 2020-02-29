#include "http_response_parser.hpp"

namespace mongols {

int http_response_parser::on_message_begin(llhttp_t* p)
{
    return 0;
}

int http_response_parser::on_message_complete(llhttp_t* p)
{
    return 0;
}

int http_response_parser::on_body(llhttp_t* p, const char* buf, size_t len)
{
    http_response_parser::tmp_* THIS = (http_response_parser::tmp_*)p->data;
    THIS->parser->body.assign(buf, len);
    return 0;
}

int http_response_parser::on_chunk_complete(llhttp_t* p)
{
    return 0;
}

int http_response_parser::on_chunk_header(llhttp_t* p)
{
    return 0;
}

int http_response_parser::on_header_field(llhttp_t* p, const char* buf, size_t len)
{
    http_response_parser::tmp_* THIS = (http_response_parser::tmp_*)p->data;
    THIS->pair.first = std::move(std::string(buf, len));
    THIS->parser->res.headers.insert(std::move(std::make_pair(THIS->pair.first, "")));
    return 0;
}

int http_response_parser::on_header_value(llhttp_t* p, const char* buf, size_t len)
{
    http_response_parser::tmp_* THIS = (http_response_parser::tmp_*)p->data;
    THIS->parser->res.headers.find(THIS->pair.first)->second = std::move(std::string(buf, len));
    return 0;
}

int http_response_parser::on_headers_complete(llhttp_t* p)
{
    return 0;
}

int http_response_parser::on_status(llhttp_t* p, const char* at, size_t length)
{
    http_response_parser::tmp_* THIS = (http_response_parser::tmp_*)p->data;
    THIS->parser->res.status = p->status_code;
    return 0;
}

int http_response_parser::on_url(llhttp_t* p, const char* buf, size_t len)
{
    return 0;
}

http_response_parser::http_response_parser(mongols::response& res)
    : tmp()
    , parser()
    , settings()
    , res(res)
    , body()
{
    res.headers.erase("Content-Type");
    llhttp_settings_init(&this->settings);

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

    llhttp_init(&this->parser, HTTP_RESPONSE, &this->settings);
    this->tmp.parser = this;
    this->parser.data = &this->tmp;
}

bool http_response_parser::parse(const std::string& str)
{
    return llhttp_execute(&this->parser, str.c_str(), str.size()) == HPE_OK;
}

bool http_response_parser::parse(const char* str, size_t len)
{
    return llhttp_execute(&this->parser, str, len) == HPE_OK;
}

const std::string& http_response_parser::get_body() const
{
    return this->body;
}

std::string& http_response_parser::get_body()
{
    return this->body;
}
}