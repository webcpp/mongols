#include "inotify.hpp"
#include <iostream>
namespace mongols
{

    size_t inotify::buffer_size = (1024 * ((sizeof(struct inotify_event)) + 16));

    inotify::inotify(const std::string &path)
        : fd(-1), wd(-1), path(path), cb()
    {
        this->fd = inotify_init1(IN_NONBLOCK);
    }

    inotify::~inotify()
    {
        if (this->fd >= 0)
        {
            if (this->wd >= 0)
            {
                inotify_rm_watch(this->fd, this->wd);
            }
            close(this->fd);
        }
    }

    int inotify::get_fd() const
    {
        return this->fd;
    }

    uint32_t inotify::get_mask() const
    {
        return this->mask;
    }

    const std::string &inotify::get_path() const
    {
        return this->path;
    }

    bool inotify::watch(uint32_t mask)
    {
        if (this->fd < 0 || this->path.empty())
        {
            return false;
        }
        this->wd = inotify_add_watch(this->fd, this->path.c_str(), mask);
        this->mask = mask;
        return this->wd >= 0;
    }

    void inotify::set_cb(const std::function<void(struct inotify_event *)> &fun)
    {
        this->cb = fun;
    }

    void inotify::run()
    {
        char buffer[inotify::buffer_size];
        size_t len, i = 0;
        len = read(this->fd, buffer, inotify::buffer_size);
        if (len < i)
        {
            return;
        }
        while (i < len)
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (this->cb)
            {
                this->cb(event);
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
} // namespace mongols