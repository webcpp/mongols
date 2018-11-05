#include <fcntl.h>          
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>

#include <cstring>         
#include <cstdlib> 


#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>

#include "tcp_threading_server.hpp"


namespace mongols {

    tcp_threading_server::tcp_threading_server(const std::string& host, int port
            , int timeout, size_t buffer_size
            , size_t thread_size, int max_event_size)
    : tcp_server(host, port
    , timeout, buffer_size
    , max_event_size)
    , main_mtx() {
        this->thread_size = (thread_size == 0 ? std::thread::hardware_concurrency() : thread_size);
    }

    void tcp_threading_server::add_client(int fd, const std::string& ip, int port) {
        std::lock_guard<std::mutex> lk(this->main_mtx);
        this->clients.insert(std::move(std::make_pair(fd, std::move(client_t(ip, port, 0, 0)))));
    }

    void tcp_threading_server::del_client(int fd) {
        std::lock_guard<std::mutex> lk(this->main_mtx);
        this->clients.erase(fd);
    }

    bool tcp_threading_server::send_to_other_client(int fd, int ffd, const client_t& client, const std::string& str, const filter_handler_function& h) {
        if (ffd != fd && h(client) && send(ffd, str.c_str(), str.size(), MSG_NOSIGNAL) < 0) {
            close(ffd);
            this->del_client(ffd);
        }
        return false;
    }

    bool tcp_threading_server::send_to_all_client(int fd, const std::string& str, const filter_handler_function& h) {
        std::lock_guard<std::mutex> lk(this->main_mtx);
        for (auto &i : this->clients) {
            this->work_pool->submit(std::bind(&tcp_threading_server::send_to_other_client, this, fd, i.first, i.second, str, h));
        }
        return false;
    }

    bool tcp_threading_server::work(int fd, const handler_function& g) {
        char buffer[this->buffer_size];
ev_recv:
        ssize_t ret = recv(fd, buffer, this->buffer_size, MSG_WAITALL);
        if (ret == -1) {
            if (errno == EINTR) {
                goto ev_recv;
            } else if (errno == EAGAIN) {
                return false;
            } else {
                goto ev_error;
            }
        } else if (ret > 0) {
            std::pair<char*, size_t> input;
            input.first = &buffer[0];
            input.second = ret;
            std::string output;
            filter_handler_function send_to_other_filter = [](const tcp_server::client_t&) {
                return true;
            };
            bool keepalive = CLOSE_CONNECTION, send_to_all = false;
            {
                std::lock_guard<std::mutex> lk(this->main_mtx);
                tcp_server::client_t& client = this->clients[fd];
                client.u_size = this->clients.size();
                output = std::move(g(input, keepalive, send_to_all, client, send_to_other_filter));
            }
            size_t n = send(fd, output.c_str(), output.size(), MSG_NOSIGNAL);
            if (n >= 0) {
                if (send_to_all) {
                    this->work_pool->submit(std::bind(&tcp_threading_server::send_to_all_client, this, fd, output, send_to_other_filter));
                }
            }
            if (n < 0 || keepalive == CLOSE_CONNECTION) {
                goto ev_error;
            }

        } else {

ev_error:
            close(fd);
            this->del_client(fd);

        }
        return false;
    }




}