#ifndef BFAC9F12_63AE_4128_BC55_38822B8FB7A0
#define BFAC9F12_63AE_4128_BC55_38822B8FB7A0

#include <functional>
#include <sys/epoll.h>

namespace mongols {

class epoll {
public:
    epoll() = delete;
    epoll(int ev_size = 64, int timeout = -1);

    virtual ~epoll();

public:
    bool is_ready() const;
    bool add(int fd, int event);
    bool del(int fd);
    bool mod(int fd, int event);
    size_t size() const;
    size_t expires() const;
    void loop(const std::function<void(struct epoll_event*)>& handler);

private:
    int epfd, ev_size, real_ev_size, timeout;
    struct epoll_event ev, *evs;
};
}

#endif /* BFAC9F12_63AE_4128_BC55_38822B8FB7A0 */
