#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "re2/re2.h"
#include "udp_server.hpp"
#include "util.hpp"

namespace mongols {
std::atomic_bool udp_server::done(true);
int udp_server::backlog = 511;
size_t udp_server::backlist_size = 1024;
size_t udp_server::clients_size = 1024;
size_t udp_server::max_connection_limit = 30;
size_t udp_server::backlist_timeout = 24 * 60 * 60;
size_t udp_server::max_send_limit = 5;

udp_server::setsockopt_function udp_server::setsockopt_cb = nullptr;

void udp_server::signal_normal_cb(int sig, siginfo_t*, void*)
{
    udp_server::done = false;
}

udp_server::udp_server(const std::string& host, int port, int timeout, size_t buffer_size, int max_event_size)
    : host(host)
    , port(port)
    , listenfd(0)
    , max_event_size(max_event_size)
    , server_is_ok(false)
    , server_hints()
    , server_info(0)
    , cleaning_fun()
    , whitelist_inotify()
    , server_epoll(0)
    , buffer_size(buffer_size)
    , timeout(timeout)
    , clients(udp_server::clients_size)
    , blacklist(udp_server::backlist_size)
    , whitelist()
    , enable_blacklist(false)
    , enable_whitelist(false)
    , enable_security_check(false)
{
    memset(&this->server_hints, 0, sizeof(this->server_hints));
    this->server_hints.ai_family = AF_UNSPEC;
    this->server_hints.ai_socktype = SOCK_DGRAM;
    this->server_hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &this->server_hints, &this->server_info) != 0) {
        perror("getaddrinfo error");
        return;
    } else {
        this->server_is_ok = true;
    }

    this->listenfd = socket(this->server_info->ai_family, this->server_info->ai_socktype, this->server_info->ai_protocol);

    int on = 1;
    setsockopt(this->listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    struct timeval send_timeout, recv_timeout;
    send_timeout.tv_sec = this->timeout;
    send_timeout.tv_usec = 0;
    setsockopt(this->listenfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    recv_timeout.tv_sec = this->timeout;
    recv_timeout.tv_usec = 0;
    setsockopt(this->listenfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

    if (udp_server::setsockopt_cb) {
        udp_server::setsockopt_cb(this->listenfd);
    }

    bind(this->listenfd, this->server_info->ai_addr, this->server_info->ai_addrlen);

    this->setnonblocking(this->listenfd);
}

udp_server::~udp_server()
{
    if (this->cleaning_fun) {
        this->cleaning_fun(this->listenfd);
    }
    if (this->server_info) {
        freeaddrinfo(this->server_info);
    }

    if (this->listenfd) {
        close(this->listenfd);
    }
}

udp_server::client_t::client_t()
    : ip()
    , port(-1)
    , t(time(0))
    , count(0)
{
}

udp_server::client_t::client_t(const std::string& ip, int port)
    : ip(ip)
    , port(port)
    , t(time(0))
    , count(0)
{
}

udp_server::meta_data_t::meta_data_t()
    : client()
{
}

udp_server::meta_data_t::meta_data_t(const std::string& ip, int port)
    : client(ip, port)
{
}
udp_server::black_ip_t::black_ip_t()
    : t(time(0))
    , count(1)
    , disallow(false)
{
}

void udp_server::run(const handler_function& g)
{
    if (!this->server_is_ok) {
        perror("server error");
        return;
    }
    std::vector<int> sigs = multi_process::signals;

    struct sigaction act;
    for (size_t i = 0; i < sigs.size(); ++i) {
        memset(&act, 0, sizeof(struct sigaction));
        sigemptyset(&act.sa_mask);
        act.sa_sigaction = udp_server::signal_normal_cb;
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
    this->server_epoll = &epoll;
    if (!epoll.add(this->listenfd, EPOLLIN | EPOLLET)) {
        perror("epoll listen error");
        return;
    }
    if (this->whitelist_inotify) {
        int whitelist_fd = this->whitelist_inotify->get_fd();
        if (!epoll.add(whitelist_fd, EPOLLIN | EPOLLET) || !this->whitelist_inotify->watch(IN_MODIFY)) {
            epoll.del(whitelist_fd);
            this->whitelist_inotify.reset();
        }
    }
    auto main_fun = std::bind(&udp_server::main_loop, this, std::placeholders::_1, std::cref(g), std::ref(epoll));
    while (udp_server::done) {
        epoll.loop(main_fun);
    }
}

void udp_server::set_shutdown(const shutdown_function& f)
{
    this->cleaning_fun = f;
}

void udp_server::set_whitelist(const std::string& ip)
{
    this->whitelist.push_back(ip);
}

void udp_server::del_whitelist(const std::string& ip)
{
    this->whitelist.remove(ip);
}

bool udp_server::read_whitelist_file(const std::string& path)
{
    if (mongols::is_file(path)) {
        this->whitelist.clear();
        std::ifstream input(path);
        if (input) {
            std::string line;
            while (std::getline(input, line)) {
                mongols::trim(std::ref(line));
                if (!line.empty() && line.front() != '#') {
                    this->whitelist.push_back(line);
                }
            }
            return true;
        }
    }
    return false;
}

void udp_server::set_whitelist_file(const std::string& path)
{
    char path_buffer[PATH_MAX];
    char* tmp = realpath(path.c_str(), path_buffer);
    std::string real_path;
    if (tmp) {
        real_path = tmp;
    } else {
        return;
    }
    size_t p = real_path.find_last_of('/');
    std::string dir = real_path.substr(0, p), file_name = real_path.substr(p + 1);
    if (this->read_whitelist_file(real_path)) {
        this->whitelist_inotify = std::make_shared<inotify>(dir);
        if (this->whitelist_inotify->get_fd() < 0) {
            this->whitelist_inotify.reset();
        } else {
            this->whitelist_inotify->set_cb([&, real_path, file_name](struct inotify_event* event) {
                if (event->len > 0) {
                    if (strncmp(event->name, file_name.c_str(), event->len) == 0 && event->mask & this->whitelist_inotify->get_mask()) {
                        this->read_whitelist_file(real_path);
                    }
                }
            });
        }
    }
}

void udp_server::setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool udp_server::check_blacklist(const std::string& ip)
{
    std::shared_ptr<black_ip_t> black_ip;
    if (this->blacklist.tryGet(ip, black_ip)) {
        double diff = difftime(time(0), black_ip->t);
        if (black_ip->disallow) {
            if (diff < udp_server::backlist_timeout) {
                return false;
            } else {
                black_ip->disallow = false;
                black_ip->count = 1;
                black_ip->t = time(0);
            }
        }

        if ((diff == 0 && black_ip->count > udp_server::max_connection_limit)
            || (diff > 0 && black_ip->count / diff > udp_server::max_connection_limit)) {
            black_ip->t = time(0);
            black_ip->disallow = true;
            return false;
        } else {
            black_ip->count++;
        }
    } else {
        this->blacklist.insert(ip, std::make_shared<black_ip_t>());
    }
    return true;
}

bool udp_server::check_whitelist(const std::string& ip)
{
    return std::find_if(this->whitelist.begin(), this->whitelist.end(), [&](const std::string& v) {
        return re2::RE2::FullMatch(ip, v);
    }) != this->whitelist.end();
}

bool udp_server::security_check(const udp_server::client_t& client)
{
    time_t now = time(0);
    double diff = difftime(now, client.t);
    if ((diff == 0 && client.count > udp_server::max_send_limit)
        || (diff > 0 && client.count / diff > udp_server::max_send_limit)) {
        return false;
    }
    return true;
}

bool udp_server::work(int fd, const handler_function& g)
{
    char buffer[this->buffer_size];
    bool rereaded = false;
    struct sockaddr_storage clientaddr;
    socklen_t clilen = sizeof(clientaddr);
ev_recv:
    ssize_t ret = recvfrom(fd, buffer, this->buffer_size, MSG_WAITALL, (struct sockaddr*)&clientaddr, &clilen);
    if (ret < 0) {
        if (errno == EINTR) {
            if (!rereaded) {
                rereaded = true;
                goto ev_recv;
            }
        } else if (errno == EAGAIN) {
            return false;
        }
        goto ev_error;

    } else if (ret > 0) {
        std::string clientip;
        int clientport = 0;
        if (!this->get_client_address(&clientaddr, clientip, clientport)) {
            goto ev_error;
        }
        std::pair<char*, size_t> input;
        input.first = &buffer[0];
        input.second = ret;

        client_t client;
        if (this->clients.tryGet(clientip, client)) {
            client.count++;
        } else {
            client.ip = clientip;
            client.port = port;
            client.count++;
            this->clients.insert(clientip, client);
        }
        if (this->enable_blacklist && !this->check_blacklist(clientip)) {
            goto ev_error;
        }
        if (this->enable_whitelist && !this->check_whitelist(clientip)) {
            goto ev_error;
        }
        if (this->enable_security_check && !this->security_check(client)) {
            goto ev_error;
        }
        std::string output = std::move(g(input, client));

        if (output.empty()) {
            goto ev_error;
        }

        ret = sendto(fd, output.c_str(), output.size(), MSG_NOSIGNAL, (struct sockaddr*)&clientaddr, clilen);

        if (ret <= 0) {
            this->clients.remove(clientip);
            goto ev_error;
        }
        return true;
    }
ev_error:
    return false;
}

void udp_server::main_loop(struct epoll_event* event, const handler_function& g, mongols::epoll& epoll)
{
    if (event->data.fd == this->listenfd) {

        while (udp_server::done) {
            this->work(event->data.fd, g);
        }

    } else if (this->whitelist_inotify && event->data.fd == this->whitelist_inotify->get_fd()) {
        this->whitelist_inotify->run();
    }
}

size_t udp_server::get_buffer_size() const
{
    return this->buffer_size;
}

bool udp_server::get_client_address(struct sockaddr_storage* address, std::string& ip, int& port)
{
    if (address->ss_family == AF_INET) {
        struct sockaddr_in* clientaddr_v4 = (struct sockaddr_in*)address;
        char clistr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clientaddr_v4->sin_addr, clistr, INET_ADDRSTRLEN)) {
            ip = clistr;
            port = ntohs(clientaddr_v4->sin_port);
            return true;
        }
    } else if (address->ss_family == AF_INET6) {
        struct sockaddr_in6* clientaddr_v6 = (struct sockaddr_in6*)address;
        char clistr[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &clientaddr_v6->sin6_addr, clistr, INET6_ADDRSTRLEN)) {
            ip = clistr;
            port = ntohs(clientaddr_v6->sin6_port);
            return true;
        }
    }
    return false;
}

void udp_server::set_enable_blacklist(bool b)
{
    this->enable_blacklist = b;
}

void udp_server::set_enable_security_check(bool b)
{
    this->enable_security_check = b;
}
void udp_server::set_enable_whitelist(bool b)
{
    this->enable_whitelist = b;
}
}