#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <netdb.h>

#include <functional>

#include "tcp_proxy_server.hpp"
#include "http_request_parser.hpp"
#include "lib/hash/md5.hpp"
#include "cache.h"


namespace mongols {

    tcp_client::tcp_client(const std::string& host, int port) : host(host), port(port), socket_fd(-1), server_addr(), server(0) {
        this->init();
    }

    tcp_client::~tcp_client() {
        if (this->socket_fd > 0) {
            close(this->socket_fd);
        }
    }

    void tcp_client::init() {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->socket_fd < 0) {
            return;
        }
        this->server = gethostbyname(this->host.c_str());
        if (server == NULL) {
            close(this->socket_fd);
            this->socket_fd = -1;
            return;
        }
        bzero((char *) & this->server_addr, sizeof (this->server_addr));
        this->server_addr.sin_family = AF_INET;
        bcopy((char *) this->server->h_addr,
                (char *) & this->server_addr.sin_addr.s_addr,
                this->server->h_length);
        this->server_addr.sin_port = htons(this->port);
        if (connect(this->socket_fd, (struct sockaddr *) & this->server_addr, sizeof (this->server_addr)) < 0) {
            close(this->socket_fd);
            this->socket_fd = -1;
        }
    }

    bool tcp_client::ok() {
        return this->socket_fd > 0;
    }

    ssize_t tcp_client::recv(char* buffer, size_t len) {
        return ::read(this->socket_fd, buffer, len);
    }

    ssize_t tcp_client::send(const char* str, size_t len) {
        return ::write(this->socket_fd, str, len);
    }

    std::string tcp_proxy_server::DEFAULT_HTTP_CONTENT = "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: 70\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html>"
            "<head><title>403</title></head>"
            "<body>403 Forbidden</body>"
            "</html>";

    tcp_proxy_server::tcp_proxy_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, int max_event_size)
    : index(0), back_end_size(0), http_lru_cache_size(1024), http_lru_cache_expires(300), enable_http(false), enable_http_lru_cache(false)
    , server(0), back_server(), clients(), default_content(), http_lru_cache(0) {
        if (thread_size > 0) {
            this->server = new tcp_threading_server(host, port, timeout, buffer_size, thread_size, max_event_size);
        } else {
            this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
        }
    }

    tcp_proxy_server::~tcp_proxy_server() {
        if (this->http_lru_cache) {
            delete this->http_lru_cache;
        }

        if (this->server) {
            delete this->server;
        }
    }

    void tcp_proxy_server::run(const tcp_server::filter_handler_function& g) {
        if (this->enable_http && this->enable_http_lru_cache) {
            this->http_lru_cache = new lru11::Cache<std::string, std::shared_ptr < std::pair<std::string, time_t> >> (this->http_lru_cache_size);
        }

        tcp_server::handler_function f = std::bind(&tcp_proxy_server::work, this
                , std::cref(g)
                , std::placeholders::_1
                , std::placeholders::_2
                , std::placeholders::_3
                , std::placeholders::_4
                , std::placeholders::_5);

        this->server->run(f);
    }

    void tcp_proxy_server::set_back_server(const std::string& host, int port) {
        this->back_server.emplace_back(std::make_pair(host, port));
        this->back_end_size++;
    }

    void tcp_proxy_server::set_default_content(const std::string& str) {
        this->default_content = str;
    }

    void tcp_proxy_server::set_enable_http_mode(bool b) {
        this->enable_http = b;
    }

    void tcp_proxy_server::set_enable_http_lru_cache(bool b) {
        this->enable_http_lru_cache = b;
    }

    void tcp_proxy_server::set_http_lru_cache_size(size_t len) {
        this->http_lru_cache_size = len;
    }

    void tcp_proxy_server::set_http_lru_cache_expires(long long expires) {
        this->http_lru_cache_expires = expires;
    }

    std::string tcp_proxy_server::work(const tcp_server::filter_handler_function& f
            , const std::pair<char*, size_t>& input, bool& keepalive
            , bool& send_to_other, tcp_server::client_t& client, tcp_server::filter_handler_function& send_to_other_filter) {
        keepalive = CLOSE_CONNECTION;
        send_to_other = false;
        std::shared_ptr<std::string> cache_key;
        std::shared_ptr<std::pair < std::string, time_t>> output;
        if (f(client)) {
            if (this->enable_http) {
                mongols::request req;
                mongols::http_request_parser parser(req);
                if (parser.parse(input.first, input.second)) {
                    std::unordered_map<std::string, std::string>::const_iterator tmp_iterator;
                    if ((tmp_iterator = req.headers.find("Connection")) != req.headers.end()) {
                        if (tmp_iterator->second == "keep-alive") {
                            keepalive = KEEPALIVE_CONNECTION;
                        }
                    }
                    if (this->enable_http_lru_cache) {
                        std::string tmp_str(req.method);
                        tmp_str.append(req.uri);
                        cache_key = std::make_shared<std::string>(std::move(mongols::md5(req.param.empty() ? tmp_str : tmp_str.append("?").append(req.param))));
                        if (this->http_lru_cache->contains(*cache_key)) {
                            output = this->http_lru_cache->get(*cache_key);
                            if (difftime(time(0), output->second)>this->http_lru_cache_expires) {
                                this->http_lru_cache->remove(*cache_key);
                            } else {
                                return output->first;
                            }
                        }
                    }

                } else {
                    goto done;
                }
            }
            std::unordered_map<size_t, std::shared_ptr < tcp_client>>::iterator iter = this->clients.find(client.sid);
            std::shared_ptr<tcp_client> cli;
            bool is_old = false;
            if (iter == this->clients.end()) {
new_client:
                if (this->index>this->back_end_size - 1) {
                    this->index = 0;
                }
                std::vector<std::pair < std::string, int>>::const_reference back_end = this->back_server[this->index++];
                cli = std::make_shared<tcp_client>(back_end.first, back_end.second);
                this->clients[client.sid] = cli;
                is_old = false;
            } else {
                cli = iter->second;
                is_old = true;
            }

            if (cli->ok()) {
                ssize_t ret = cli->send(input.first, input.second);
                if (ret > 0) {
                    char buffer[this->server->get_buffer_size()];
                    ret = cli->recv(buffer, this->server->get_buffer_size());
                    if (ret > 0) {
                        if (this->enable_http && this->enable_http_lru_cache) {
                            output = std::make_shared<std::pair < std::string, time_t >> ();
                            output->first.assign(buffer, ret);
                            output->second = time(0);
                            this->http_lru_cache->insert(*cache_key, output);
                            return output->first;
                        }
                        keepalive = KEEPALIVE_CONNECTION;
                        return std::string(buffer, ret);
                    }
                }
            }
            this->clients.erase(client.sid);
            if (is_old) {
                goto new_client;
            }
        }
done:
        return this->default_content;
    }

    void tcp_proxy_server::set_default_http_content() {
        this->default_content = tcp_proxy_server::DEFAULT_HTTP_CONTENT;
    }







}
