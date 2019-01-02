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
    

    tcp_proxy_server::tcp_proxy_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, int max_event_size)
    : index(0), back_end_size(0), server(0), back_server(), clients(), default_content() {
        this->server = new tcp_server(host, port, timeout, buffer_size, max_event_size);
    }

    tcp_proxy_server::~tcp_proxy_server() {
        if (this->server) {
            delete this->server;
        }
    }

    void tcp_proxy_server::run(const tcp_server::filter_handler_function& g) {
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

    std::string tcp_proxy_server::work(const tcp_server::filter_handler_function& f
            , const std::pair<char*, size_t>& input, bool& keepalive
            , bool& send_to_other, tcp_server::client_t& client, tcp_server::filter_handler_function& send_to_other_filter) {
        keepalive = KEEPALIVE_CONNECTION;
        send_to_other = false;
        if (f(client)) {
            std::unordered_map<size_t, std::shared_ptr < tcp_client>>::iterator iter = this->clients.find(client.sid);
            std::shared_ptr<tcp_client> cli;
            if (iter == this->clients.end()) {
                if (this->index>this->back_end_size - 1) {
                    this->index = 0;
                }
                std::vector<std::pair < std::string, int>>::const_reference back_end = this->back_server[this->index++];
                cli = std::make_shared<tcp_client>(back_end.first, back_end.second);
                this->clients[client.sid] = cli;
            } else {
                cli = iter->second;
            }

            if (cli->ok()) {
                ssize_t ret = cli->send(input.first, input.second);
                if (ret > 0) {
                    char buffer[4096];
                    ret = cli->recv(buffer, 4096);
                    if (ret > 0) {
                        return std::string(buffer, ret);
                    }
                }
            }
            this->clients.erase(client.sid);
        }
        keepalive = CLOSE_CONNECTION;
        return this->default_content;
    }








}
