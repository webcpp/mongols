#pragma once

#include <atomic>
#include <ctime>
#include <list>
#include <netdb.h>
#include <queue>
#include <string>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <utility>

#include "epoll.hpp"
#include "inotify.hpp"
#include "lib/LRUCache11.hpp"

namespace mongols
{
    class udp_server
    {
    public:
        class client_t
        {
        public:
            client_t();
            client_t(const std::string &ip, int port);
            virtual ~client_t() = default;
            std::string ip;
            int port;
            time_t t;
            size_t count;
        };
        typedef std::function<void(int)> setsockopt_function;
        typedef std::function<std::string(
            const std::pair<char *, size_t> &, const udp_server::client_t &)>
            handler_function;
        typedef std::function<void(int)> shutdown_function;

        static setsockopt_function setsockopt_cb;

    public:
        udp_server() = delete;
        udp_server(const std::string &host, int port, int timeout = 10, size_t buffer_size = 8192, int max_event_size = 64);
        virtual ~udp_server();
        void run(const handler_function &);

        size_t get_buffer_size() const;

        void set_enable_blacklist(bool);
        void set_enable_security_check(bool);
        void set_enable_whitelist(bool);
        void set_shutdown(const shutdown_function &);

        virtual void set_whitelist(const std::string &);
        virtual void del_whitelist(const std::string &);
        void set_whitelist_file(const std::string &);

        static int backlog;
        static size_t backlist_size, clients_size;
        static size_t max_connection_limit;
        static size_t backlist_timeout;
        static size_t max_send_limit;

    private:
        std::string host;
        int port, listenfd, max_event_size;
        bool server_is_ok;
        struct addrinfo server_hints;
        struct addrinfo *server_info;
        shutdown_function cleaning_fun;
        std::shared_ptr<inotify> whitelist_inotify;
        static std::atomic_bool done;
        static void signal_normal_cb(int sig, siginfo_t *, void *);
        void setnonblocking(int fd);
        void main_loop(struct epoll_event *, const handler_function &, mongols::epoll &);
        bool get_client_address(struct sockaddr_storage *, std::string &, int &);

    protected:
        class meta_data_t
        {
        public:
            meta_data_t();
            meta_data_t(const std::string &ip, int port);
            virtual ~meta_data_t() = default;

        public:
            client_t client;
        };
        class black_ip_t
        {
        public:
            black_ip_t();
            virtual ~black_ip_t() = default;
            time_t t;
            size_t count;
            bool disallow;
        };
        mongols::epoll *server_epoll;
        size_t buffer_size;
        int timeout;

        lru11::Cache<std::string, client_t> clients;
        lru11::Cache<std::string, std::shared_ptr<black_ip_t>> blacklist;
        std::list<std::string> whitelist;

        bool enable_blacklist, enable_whitelist, enable_security_check;

        virtual bool work(int, const handler_function &);

        virtual bool check_blacklist(const std::string &);
        virtual bool check_whitelist(const std::string &);
        virtual bool read_whitelist_file(const std::string &);
        virtual bool security_check(const udp_server::client_t &);
    };
} // namespace mongols
