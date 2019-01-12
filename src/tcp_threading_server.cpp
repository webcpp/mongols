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

    bool tcp_threading_server::add_client(int fd, const std::string& ip, int port) {
        std::lock_guard<std::mutex> lk(this->main_mtx);
        auto pair = this->clients.insert(std::move(std::make_pair(fd, std::move(client_t(ip, port, 0, 0)))));
        if (this->sid_queue.empty()) {
            pair.first->second.sid = ++this->sid;
        } else {
            pair.first->second.sid = this->sid_queue.front();
            this->sid_queue.pop();
        }
        if (this->openssl_is_ok) {
            std::shared_ptr<openssl::ssl> ssl = std::make_shared<openssl::ssl>(this->openssl_manager->get_ctx());
            if (this->openssl_manager->set_socket_and_accept(ssl->get_ssl(), fd)) {
                this->ssl_map[fd] = std::move(ssl);
            } else {
                return false;
            }
        }
        return true;
    }

    void tcp_threading_server::del_client(int fd) {
        std::lock_guard<std::mutex> lk(this->main_mtx);
        this->sid_queue.push(this->clients.find(fd)->second.sid);
        this->clients.erase(fd);
        if (this->openssl_is_ok) {
            this->ssl_map.erase(fd);
        }
    }

    bool tcp_threading_server::send_to_other_client(int fd, int ffd, const client_t& client, const std::string& str, const filter_handler_function& h) {
        if (ffd != fd && h(client) &&
                (this->openssl_is_ok
                ? this->openssl_manager->write(this->ssl_map[ffd]->get_ssl(), str) < 0
                : send(ffd, str.c_str(), str.size(), MSG_NOSIGNAL) < 0)
                ) {
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
                client.count++;
                output = std::move(g(input, keepalive, send_to_all, client, send_to_other_filter));
            }
            ret = send(fd, output.c_str(), output.size(), MSG_NOSIGNAL);
            if (ret > 0) {
                if (send_to_all) {
                    this->work_pool->submit(std::bind(&tcp_threading_server::send_to_all_client, this, fd, output, send_to_other_filter));
                }
            }
            if (ret <= 0 || keepalive == CLOSE_CONNECTION) {
                goto ev_error;
            }

        } else {

ev_error:
            close(fd);
            this->del_client(fd);

        }
        return false;
    }

    bool tcp_threading_server::ssl_work(int fd, const handler_function& g) {
        char buffer[this->buffer_size];
        ssize_t ret = 0;
ev_recv:
        {
            std::lock_guard<std::mutex> lk(this->main_mtx);
            if (difftime(time(0), this->ssl_map[fd]->get_time()) > this->timeout) {
                goto ev_error;
            }
            ret = this->openssl_manager->read(this->ssl_map[fd]->get_ssl(), buffer, this->buffer_size);}
        if (ret == -1) {
            std::lock_guard<std::mutex> lk(this->main_mtx);
            int err = SSL_get_error(this->ssl_map[fd]->get_ssl(), ret);
            switch (err) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    return false;
                case SSL_ERROR_SYSCALL:
                    switch (errno) {
                        case EINTR:
                            goto ev_recv;
                        case EAGAIN:
                            return false;
                        default:
                            goto ev_error;
                    }
                default:
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
                client.count++;
                output = std::move(g(input, keepalive, send_to_all, client, send_to_other_filter));

                ret = this->openssl_manager->write(this->ssl_map[fd]->get_ssl(), output);
            }
            if (ret > 0) {
                if (send_to_all) {
                    this->work_pool->submit(std::bind(&tcp_threading_server::send_to_all_client, this, fd, output, send_to_other_filter));
                }
            }
            if (ret <= 0 || keepalive == CLOSE_CONNECTION) {
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