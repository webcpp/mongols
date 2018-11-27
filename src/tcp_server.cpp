#include <fcntl.h>          
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include <cstring>         
#include <cstdlib> 


#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include <atomic>


#include "tcp_server.hpp"



namespace mongols {

    std::atomic_bool tcp_server::done(true);

    void tcp_server::signal_normal_cb(int sig, siginfo_t *, void *) {
        switch (sig) {
            case SIGTERM:
            case SIGHUP:
            case SIGQUIT:
            case SIGINT:
                tcp_server::done = false;
                break;
            default:break;
        }
    }

    tcp_server::tcp_server(const std::string& host
            , int port
            , int timeout
            , size_t buffer_size
            , int max_event_size) :
    host(host), port(port), listenfd(0), timeout(timeout), max_event_size(max_event_size), serveraddr()
    , buffer_size(buffer_size), thread_size(0), clients(), work_pool(0) {
        this->listenfd = socket(AF_INET, SOCK_STREAM, 0);

        int on = 1;
        setsockopt(this->listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on));

        struct timeval send_timeout, recv_timeout;
        send_timeout.tv_sec = this->timeout;
        send_timeout.tv_usec = 0;
        setsockopt(this->listenfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof (send_timeout));

        recv_timeout.tv_sec = this->timeout;
        recv_timeout.tv_usec = 0;
        setsockopt(this->listenfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof (recv_timeout));



        memset(&this->serveraddr, 0, sizeof (this->serveraddr));
        this->serveraddr.sin_family = AF_INET;
        inet_aton(this->host.c_str(), &serveraddr.sin_addr);
        this->serveraddr.sin_port = htons(this->port);
        bind(this->listenfd, (struct sockaddr*) & this->serveraddr, sizeof (this->serveraddr));

        this->setnonblocking(this->listenfd);

        listen(this->listenfd, 10);

    }

    tcp_server::~tcp_server() {
        if (this->work_pool) {
            delete this->work_pool;
        }
    }

    tcp_server::client_t::client_t() : ip(), port(-1), uid(0), u_size(0), gid() {
        this->gid.push_back(0);
    }

    tcp_server::client_t::client_t(const std::string& ip, int port, size_t uid, size_t gid)
    : ip(ip), port(port), uid(uid), u_size(0), gid() {
        this->gid.push_back(gid);
    }

    void tcp_server::run(const handler_function& g) {
        std::vector<int> sigs = {SIGHUP, SIGTERM, SIGINT, SIGQUIT};

        struct sigaction act;
        for (size_t i = 0; i < sigs.size(); ++i) {
            memset(&act, 0, sizeof (struct sigaction));
            sigemptyset(&act.sa_mask);
            act.sa_sigaction = tcp_server::signal_normal_cb;
            act.sa_flags = SA_SIGINFO;
            if (sigaction(sigs[i], &act, NULL) < 0) {
                perror("sigaction error");
                return;
            }
        }
        mongols::epoll epoll(this->max_event_size, -1);
        if (!epoll.is_ready()) {
            perror("epoll error");
            return;
        }
        epoll.add(this->listenfd, EPOLLIN | EPOLLET);
        auto main_fun = std::bind(&tcp_server::main_loop, this, std::placeholders::_1, std::cref(g), std::ref(epoll));
        if (this->thread_size > 0) {
            this->work_pool = new mongols::thread_pool < std::function<bool() >>(this->thread_size);
        }
        while (tcp_server::done) {
            epoll.loop(main_fun);
        }
    }

    void tcp_server::setnonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    void tcp_server::add_client(int fd, const std::string& ip, int port) {
        this->clients.insert(std::move(std::make_pair(fd, std::move(client_t(ip, port, 0, 0)))));
    }

    void tcp_server::del_client(int fd) {
        this->clients.erase(fd);
    }

    bool tcp_server::send_to_all_client(int fd, const std::string& str, const filter_handler_function& h) {
        for (auto i = this->clients.begin(); i != this->clients.end();) {
            if (i->first != fd && h(i->second) && send(i->first, str.c_str(), str.size(), MSG_NOSIGNAL) < 0) {
                close(i->first);
                this->del_client(i->first);
            } else {
                ++i;
            }
        }
        return false;
    }

    bool tcp_server::work(int fd, const handler_function& g) {
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
            filter_handler_function send_to_other_filter = [](const client_t&) {
                return true;
            };

            bool keepalive = CLOSE_CONNECTION, send_to_all = false;
            client_t& client = this->clients[fd];
            client.u_size = this->clients.size();
            std::string output = std::move(g(input, keepalive, send_to_all, client, send_to_other_filter));
            size_t n = send(fd, output.c_str(), output.size(), MSG_NOSIGNAL);
            if (n >= 0) {
                if (send_to_all) {
                    this->send_to_all_client(fd, output, send_to_other_filter);
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

    void tcp_server::main_loop(struct epoll_event * event
            , const handler_function& g
            , mongols::epoll& epoll) {
        if (event->data.fd == this->listenfd) {
            while (tcp_server::done) {
                struct sockaddr_in clientaddr;
                socklen_t clilen;
                int connfd = accept(listenfd, (struct sockaddr*) &clientaddr, &clilen);
                if (connfd > 0) {
                    this->setnonblocking(connfd);
                    epoll.add(connfd, EPOLLIN | EPOLLRDHUP | EPOLLET);
                    this->add_client(connfd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                } else {
                    break;
                }
            }
        } else if (event->events & EPOLLIN) {
            this->work(event->data.fd, g);
        } else {
            close(event->data.fd);
        }
    }

    size_t tcp_server::get_buffer_size() const {
        return this->buffer_size;
    }





}