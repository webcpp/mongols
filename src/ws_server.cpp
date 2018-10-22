#include <algorithm>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>

#include "ws_server.hpp"
#include "lib/websocket_parser.h"
#include "lib/libwshandshake.hpp"
#include "json/json.h"





namespace mongols {

    ws_server::ws_server(const std::string& host, int port, int timeout
            , size_t buffer_size, size_t thread_size, int max_event_size)
    : server(0) {
        if (thread_size > 0) {
            this->server = new tcp_threading_server(host, port, timeout, buffer_size, thread_size, max_event_size);
        } else {
            this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
        }
    }

    ws_server::~ws_server() {
        if (this->server) {
            delete this->server;
        }
    }

    void ws_server::run(const message_handler_function& f) {
        this->server->run(std::bind(&ws_server::work, this
                , std::cref(f)
                , std::placeholders::_1
                , std::placeholders::_2
                , std::placeholders::_3
                , std::placeholders::_4
                , std::placeholders::_5));
    }

    void ws_server::run() {
        message_handler_function f = std::bind(&ws_server::ws_json_parse, this
                , std::placeholders::_1
                , std::placeholders::_2
                , std::placeholders::_3
                , std::placeholders::_4
                , std::placeholders::_5
                );
        this->server->run(std::bind(&ws_server::work, this
                , f
                , std::placeholders::_1
                , std::placeholders::_2
                , std::placeholders::_3
                , std::placeholders::_4
                , std::placeholders::_5
                ));
    }

    std::string ws_server::ws_json_parse(const std::string& message
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function& send_to_other_filter) {

        Json::Reader json_reader;
        Json::Value root;
        if (json_reader.parse(message, root)) {
            if (root.isMember("uid")
                    && root.isMember("gid")
                    && root.isMember("ufilter")
                    && root["ufilter"].isArray()
                    && root.isMember("gfilter")
                    && root["gfilter"].isArray()) {

                int64_t uid = root["uid"].asInt64();
                if (client.uid == 0 && uid > 0) {
                    client.uid = uid;
                }

                std::function<void(const Json::Value &) > gid_filter = [&](const Json::Value & v) {
                    if (v.isNumeric()) {
                        int64_t gid = v.asInt64();
                        bool gid_is_uint64 = (gid >= 0);
                        std::list<size_t>::iterator gid_iter = std::find(client.gid.begin(), client.gid.end(), (gid_is_uint64 ? gid : -gid));
                        if (gid_iter == client.gid.end()) {
                            if (gid_is_uint64) {
                                client.gid.emplace_back(gid);
                            }
                        } else {
                            if (!gid_is_uint64) {
                                client.gid.erase(gid_iter);
                            }
                        }
                    } else if (v.isArray()) {
                        Json::ArrayIndex gid_v_size = v.size();
                        for (Json::ArrayIndex i = 0; i < gid_v_size; ++i) {
                            gid_filter(v[i]);

                        }
                    }
                };
                const Json::Value& gid_v = root["gid"];
                gid_filter(gid_v);


                keepalive = KEEPALIVE_CONNECTION;
                send_to_other = true;
                std::vector<size_t> gfilter, ufilter;
                const Json::Value& gfilter_array = root["gfilter"], &ufilter_array = root["ufilter"];
                Json::ArrayIndex ufilter_size = ufilter_array.size(), gfilter_size = gfilter_array.size();
                for (Json::ArrayIndex i = 0; i < ufilter_size; ++i) {
                    if (ufilter_array[i].isUInt64()) {
                        ufilter.emplace_back(ufilter_array[i].asUInt64());
                    }
                }
                for (Json::ArrayIndex i = 0; i < gfilter_size; ++i) {
                    if (gfilter_array[i].isUInt64()) {
                        gfilter.emplace_back(gfilter_array[i].asUInt64());
                    }
                }
                send_to_other_filter = [ = ](const tcp_server::client_t & cur_client){
                    bool res = false;
                    if (gfilter.empty()) {
                        if (ufilter.empty()) {
                            res = true;
                        } else {
                            res = std::find(ufilter.begin(), ufilter.end(), cur_client.uid) != ufilter.end();
                        }
                    } else {
                        auto tmp_fun = [&](size_t i) {
                            return std::find(cur_client.gid.begin(), cur_client.gid.end(), i) != cur_client.gid.end();
                        };
                        if (ufilter.empty()) {
                            res = std::find_if(gfilter.begin(), gfilter.end(), tmp_fun) != gfilter.end();
                        } else {
                            res = std::find(ufilter.begin(), ufilter.end(), cur_client.uid) != ufilter.end()
                                    && std::find_if(gfilter.begin(), gfilter.end(), tmp_fun) != gfilter.end();
                        }
                    }
                    return res;

                };
                root["ip"] = client.ip;
                root["u_size"] = client.u_size;
                Json::FastWriter json_writer;
                return json_writer.write(root);
            }
        } else {
            keepalive = CLOSE_CONNECTION;
            send_to_other = false;
        }
        return message;
    }

    std::string ws_server::work(const message_handler_function& f
            , const std::pair<char*, size_t>& input
            , bool& keepalive
            , bool& send_to_other
            , tcp_server::client_t& client
            , tcp_server::filter_handler_function& send_to_other_filter) {
        std::string response;
        keepalive = KEEPALIVE_CONNECTION;
        send_to_other = false;

        if (input.first[0] == 'G') {
            std::unordered_map<std::string, std::string> headers;
            std::unordered_map<std::string, std::string>::const_iterator headers_iterator;
            if (!this->ws_handshake(input, response, headers)) {
                keepalive = CLOSE_CONNECTION;
            } else if ((headers_iterator = headers.find("X-Real-IP")) != headers.end()) {
                client.ip = headers_iterator->second;
            }
            goto ws_done;
        } else {

            std::string close_msg = "connection closed.", pong_msg = "pong", ping_msg = "ping", error_msg = "error message."
                    , binary_msg = "not accept binary message.", message;

            int ret = this->ws_parse(input, message);


            if (ret == 1) {
                message = std::move(f(message, keepalive, send_to_other, client, send_to_other_filter));
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_TEXT | WS_FINAL_FRAME), message.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_TEXT | WS_FINAL_FRAME), NULL, message.c_str(), message.size());
                response.assign(frame, frame_len);
                free(frame);
            } else if (ret == 2) {
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_BINARY | WS_FINAL_FRAME), message.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_BINARY | WS_FINAL_FRAME), NULL, message.c_str(), message.size());
                response.assign(frame, frame_len);
                free(frame);
                send_to_other = true;
                keepalive = KEEPALIVE_CONNECTION;
            } else if (ret == 8) {
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_CLOSE | WS_FINAL_FRAME), close_msg.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_CLOSE | WS_FINAL_FRAME), NULL, close_msg.c_str(), close_msg.size());
                response.assign(frame, frame_len);
                free(frame);
                send_to_other = false;
                keepalive = CLOSE_CONNECTION;

            } else if (ret == 9) {
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_PONG | WS_FINAL_FRAME), pong_msg.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_PONG | WS_FINAL_FRAME), NULL, pong_msg.c_str(), pong_msg.size());
                response.assign(frame, frame_len);
                free(frame);
                keepalive = KEEPALIVE_CONNECTION;
                send_to_other = false;
            } else if (ret == 10) {
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_PING | WS_FINAL_FRAME), ping_msg.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_PING | WS_FINAL_FRAME), NULL, ping_msg.c_str(), ping_msg.size());
                response.assign(frame, frame_len);
                free(frame);
                keepalive = KEEPALIVE_CONNECTION;
                send_to_other = false;
            } else if (ret == 0) {
                size_t frame_len = websocket_calc_frame_size((websocket_flags) (WS_OP_TEXT | WS_FINAL_FRAME), error_msg.size());
                char * frame = (char*) malloc(sizeof (char) * frame_len);
                frame_len = websocket_build_frame(frame, (websocket_flags) (WS_OP_TEXT | WS_FINAL_FRAME), NULL, error_msg.c_str(), error_msg.size());
                response.assign(frame, frame_len);
                free(frame);
                keepalive = CLOSE_CONNECTION;
                send_to_other = false;
            }
        }
ws_done:
        return response;
    }

    bool ws_server::ws_handshake(const std::pair<char*, size_t>& request, std::string& response, std::unordered_map<std::string, std::string>& headers) {
        bool ret = false;
        std::istringstream stream(std::string(request.first, request.second));
        std::string reqType;
        std::getline(stream, reqType);
        if (reqType.substr(0, 4) != "GET ") {
            return ret;
        }

        std::string header;
        std::string::size_type pos = 0;
        std::string websocketKey;
        while (std::getline(stream, header) && header != "\r") {
            header.erase(header.end() - 1);
            pos = header.find(": ", 0);
            if (pos != std::string::npos) {
                std::string key = header.substr(0, pos);
                std::string value = header.substr(pos + 2);
                headers.insert(std::make_pair(key, value));
                if (key == "Sec-WebSocket-Key") {
                    ret = true;
                    websocketKey = value;
                    break;
                }
            }
        }

        if (!ret) {
            return ret;
        }

        response = "HTTP/1.1 101 Switching Protocols\r\n";
        response += "Upgrade: websocket\r\n";
        response += "Connection: upgrade\r\n";
        response += "Sec-WebSocket-Accept: ";



        char output[29] = {};
        WebSocketHandshake::generate(websocketKey.c_str(), output);
        std::string serverKey = std::move(output);
        serverKey += "\r\n\r\n";
        response += serverKey;

        return ret;
    }

    int ws_server::ws_parse(const std::pair<char*, size_t>& frame, std::string& message) {

        if (frame.second == 0) {
            return 0;
        }

        struct ws_frame_t {
            websocket_flags opcode, is_final;
            char* body;
            std::string* message;
        };
        ws_frame_t ws_frame;
        ws_frame.message = &message;
        ws_frame.body = 0;

        websocket_parser_settings ws_settings;
        websocket_parser_settings_init(&ws_settings);

        ws_settings.on_frame_header = [](websocket_parser * parser) {
            ws_frame_t* THIS = (ws_frame_t*) parser->data;
            THIS->opcode = (websocket_flags) (parser->flags & WS_OP_MASK);
            THIS->is_final = (websocket_flags) (parser->flags & WS_FIN);
            if (parser->length) {
                THIS->body = (char*) malloc(parser->length);
            }
            return 0;
        };
        ws_settings.on_frame_end = [](websocket_parser * parser) {
            ws_frame_t* THIS = (ws_frame_t*) parser->data;
            if (THIS->body) {
                free(THIS->body);
            }
            return 0;
        };

        ws_settings.on_frame_body = [](websocket_parser * parser, const char *at, size_t length) {
            ws_frame_t* THIS = (ws_frame_t*) parser->data;
            if (parser->flags & WS_HAS_MASK) {
                websocket_parser_decode(&THIS->body[parser->offset], at, length, parser);
                THIS->message->assign(&THIS->body[parser->offset], length);
            } else {
                THIS->message->assign(at, length);
            }
            return 0;
        };


        websocket_parser ws_parser;
        websocket_parser_init(&ws_parser);
        ws_parser.data = &ws_frame;

        size_t nread = websocket_parser_execute(&ws_parser, &ws_settings, frame.first, frame.second);

        if (nread != frame.second) {
            return -1;
        }
        int ret = 0;
        if (ws_frame.is_final == WS_FIN) {
            if (ws_frame.opcode == WS_OP_TEXT) {
                ret = 1;
            } else if (ws_frame.opcode == WS_OP_BINARY) {
                ret = 2;
            } else if (ws_frame.opcode == WS_OP_PING) {
                ret = 9;
            } else if (ws_frame.opcode == WS_OP_CLOSE) {
                ret = 8;
            } else if (ws_frame.opcode == WS_OP_PONG) {
                ret = 10;
            }
        }
        return ret;


    }



}
