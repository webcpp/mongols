
#include "http_request_parser.hpp"


namespace mongols {

    int http_request_parser::on_message_begin(http_parser* p) {
        return 0;
    }

    int http_request_parser::on_message_complete(http_parser* p) {
        return 0;
    }

    int http_request_parser::on_body(http_parser* p, const char* buf, size_t len) {
        http_request_parser::tmp_* THIS = (http_request_parser::tmp_*) p->data;
        THIS->parser->body.assign(buf, len);
        return 0;
    }

    int http_request_parser::on_chunk_complete(http_parser* p) {
        return 0;
    }

    int http_request_parser::on_chunk_header(http_parser* p) {
        return 0;
    }

    int http_request_parser::on_header_field(http_parser* p, const char* buf, size_t len) {
        http_request_parser::tmp_* THIS = (http_request_parser::tmp_*) p->data;
        THIS->pair.first = std::move(std::string(buf, len));
        THIS->parser->req.headers.insert(std::move(std::make_pair(THIS->pair.first, "")));
        return 0;
    }

    int http_request_parser::on_header_value(http_parser* p, const char* buf, size_t len) {
        http_request_parser::tmp_* THIS = (http_request_parser::tmp_*) p->data;
        THIS->parser->req.headers[THIS->pair.first] = std::move(std::string(buf, len));
        return 0;
    }

    int http_request_parser::on_headers_complete(http_parser* p) {
        http_request_parser::tmp_* THIS = (http_request_parser::tmp_*) p->data;
        THIS->parser->req.method = std::move(http_method_str((enum http_method)p->method));
        struct http_parser_url u;
        http_parser_url_init(&u);
        http_parser_parse_url(THIS->parser->req.uri.c_str(), THIS->parser->req.uri.size(), 0, &u);
        std::string path, param;
        if (u.field_set & (1 << UF_PATH)) {
            path = std::move(THIS->parser->req.uri.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len));
        }
        if (u.field_set & (1 << UF_QUERY)) {
            param = std::move(THIS->parser->req.uri.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len));
        }
        THIS->parser->req.param = std::move(param);
        THIS->parser->req.uri = std::move(path);
        return 0;
    }

    int http_request_parser::on_status(http_parser* p, const char* at, size_t length) {
        return 0;
    }

    int http_request_parser::on_url(http_parser* p, const char* buf, size_t len) {
        http_request_parser::tmp_* THIS = (http_request_parser::tmp_*) p->data;
        THIS->parser->req.uri.assign(buf, len);
        return 0;
    }

    http_request_parser::http_request_parser(mongols::request& req)
    : tmp(), parser(), settings(), req(req), body() {
        http_parser_init(&this->parser, HTTP_REQUEST);
        http_parser_settings_init(&this->settings);
        this->tmp.parser = this;
        this->parser.data = &this->tmp;


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

    }

    bool http_request_parser::parse(const std::string& str) {
        return http_parser_execute(&this->parser, &this->settings, str.c_str(), str.size()) == str.size();
    }

    bool http_request_parser::parse(const char* str, size_t len) {
        return http_parser_execute(&this->parser, &this->settings, str, len) == len;
    }

    const std::string& http_request_parser::get_body() const {
        return this->body;
    }

    std::string& http_request_parser::get_body() {
        return this->body;
    }
}