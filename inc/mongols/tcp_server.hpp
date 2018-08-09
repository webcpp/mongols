#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP


#include <netinet/in.h>   
#include <string>
#include <utility>
#include <unordered_map>
#include <atomic>



#include "epoll.hpp"

#define CLOSE_CONNECTION true
#define KEEPALIVE_CONNECTION false

namespace mongols {

    class tcp_server {
    public:
        typedef std::function<bool(const std::pair<size_t, size_t>&) > filter_handler_function;
        typedef std::function<std::string(
                const std::string&
                , bool&
                , bool&
                , std::pair<size_t, size_t>&
                , filter_handler_function&) > handler_function;

    public:
        tcp_server() = delete;
        tcp_server(const std::string& host, int port, int timeout = 5000
                , size_t buffer_size = 1024
                , int max_event_size = 64);
        virtual~tcp_server() = default;

    public:
        void run(const handler_function&);

        size_t get_buffer_size()const;
    private:
        mongols::epoll epoll;
        std::string host;
        int port, listenfd, timeout;
        struct sockaddr_in serveraddr;

        static std::atomic_bool done;
        static void signal_normal_cb(int sig);
        void setnonblocking(int fd);
        void main_loop(struct epoll_event *, const handler_function&);

    protected:
        size_t buffer_size;
        std::unordered_map<int, std::pair<size_t, size_t> > clients;
        virtual void add_client(int);
        virtual void del_client(int);
        virtual bool send_to_all_client(int, const std::string&, const filter_handler_function&);
        virtual bool work(int, const handler_function&);
        virtual void process(int, const handler_function&);
    };
}


#endif /* TCP_SERVER_HPP */

