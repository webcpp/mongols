#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP


#include <netinet/in.h>   
#include <sys/signal.h>
#include <string>
#include <utility>
#include <unordered_map>
#include <atomic>
#include <list>
#include <thread>


#include "epoll.hpp"
#include "thread_pool.hpp"

#define CLOSE_CONNECTION true
#define KEEPALIVE_CONNECTION false

namespace mongols {

    class tcp_server {
    public:

        class client_t {
        public:

            client_t() : ip(), port(-1), uid(0), u_size(0), gid() {
                this->gid.push_back(0);
            }

            client_t(const std::string& ip, int port, size_t uid, size_t gid)
            : ip(ip), port(port), uid(uid), u_size(0), gid() {
                this->gid.push_back(gid);
            }
            virtual~client_t() = default;
            std::string ip;
            int port;
            size_t uid, u_size;
            std::list<size_t> gid;
        };
        typedef std::function<bool(const client_t&) > filter_handler_function;
        typedef std::function<std::string(
                const std::pair<char*, size_t>&
                , bool&
                , bool&
                , client_t&
                , filter_handler_function&) > handler_function;


    public:
        tcp_server() = delete;
        tcp_server(const std::string& host, int port, int timeout = 5000
                , size_t buffer_size = 8192
                , int max_event_size = 64);
        virtual~tcp_server();

    public:
        void run(const handler_function&);

        size_t get_buffer_size()const;
    private:
        std::string host;
        int port, listenfd, timeout, max_event_size;
        struct sockaddr_in serveraddr;

        static std::atomic_bool done;
        static void signal_normal_cb(int sig, siginfo_t *, void *);
        void setnonblocking(int fd);
        void main_loop(struct epoll_event *, const handler_function&, mongols::epoll&);
    protected:
        size_t buffer_size, thread_size;
        std::unordered_map<int, client_t > clients;
        mongols::thread_pool<std::function<bool() >> *work_pool;
        virtual void add_client(int, const std::string&, int);
        virtual void del_client(int);
        virtual bool send_to_all_client(int, const std::string&, const filter_handler_function&);
        virtual bool work(int, const handler_function&);
    };
}


#endif /* TCP_SERVER_HPP */

