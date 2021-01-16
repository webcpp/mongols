#pragma once

#include "util.hpp"
#include <cmath>
#include <fcntl.h>
#include <functional>
#include <map>
#include <memory>
#include <pthread.h>
#include <set>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace mongols
{

    class shared_mutex
    {
    public:
        shared_mutex()
            : mtx(0), mtx_attr(0)
        {
            this->mtx = (pthread_mutex_t *)mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (this->mtx != MAP_FAILED)
            {
                this->mtx_attr = (pthread_mutexattr_t *)mmap(0, sizeof(pthread_mutexattr_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (this->mtx_attr != MAP_FAILED)
                {
                    pthread_mutexattr_init(this->mtx_attr);
                    pthread_mutexattr_setpshared(this->mtx_attr, PTHREAD_PROCESS_SHARED);
                    pthread_mutexattr_settype(this->mtx_attr, PTHREAD_MUTEX_DEFAULT);
                    pthread_mutex_init(this->mtx, this->mtx_attr);
                }
            }
        }
        virtual ~shared_mutex()
        {
            if (this->mtx)
                pthread_mutex_destroy(this->mtx);
            if (this->mtx_attr)
                pthread_mutexattr_destroy(this->mtx_attr);
            if (this->mtx_attr)
                munmap(this->mtx_attr, sizeof(pthread_mutexattr_t));
            if (this->mtx)
                munmap(this->mtx, sizeof(pthread_mutex_t));
        }

    public:
        void lock()
        {
            pthread_mutex_lock(this->mtx);
        }
        void unlock()
        {
            pthread_mutex_unlock(this->mtx);
        }
        bool is_ok()
        {
            return this->mtx != MAP_FAILED && this->mtx_attr != MAP_FAILED;
        }
        bool is_ok() const
        {
            return this->mtx != MAP_FAILED && this->mtx_attr != MAP_FAILED;
        }

    private:
        pthread_mutex_t *mtx;
        pthread_mutexattr_t *mtx_attr;
    };

    template <typename T, size_t len = 1>
    class shared_memory
    {
    public:
        shared_memory()
            : ok(false), data(0), length(len), mtx()
        {
            if (this->mtx.is_ok())
            {
                if (this->data == 0)
                {
                    this->data = (T *)mmap(0, this->length * sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                    if (this->data != MAP_FAILED)
                    {
                        this->ok = true;
                    }
                }
            }
        }

        virtual ~shared_memory()
        {
            if (this->data)
                munmap(this->data, this->length * sizeof(T));
            this->ok = false;
        }

        bool is_ok() const
        {
            return this->ok;
        }

        bool is_ok()
        {
            return this->ok;
        }

        size_t size()
        {
            return this->length;
        }

        size_t size() const
        {
            return this->length;
        }

        void run(const std::function<void(T *, size_t)> &f)
        {
            this->mtx.lock();
            f(this->data, this->length);
            this->mtx.unlock();
        }

        T *get()
        {
            return this->data;
        }

        void lock()
        {
            this->mtx.lock();
        }

        void unlock()
        {
            this->mtx.unlock();
        }

    private:
        bool ok;
        T *data;
        size_t length;
        mongols::shared_mutex mtx;
    };

    class shared_file_memory
    {
    private:
        std::string file_name;
        int fd;
        size_t len;
        char *ptr;
        bool ok, rm_file;
        mongols::shared_mutex mtx;

    public:
        shared_file_memory() = delete;
        shared_file_memory(const std::string &file)
            : file_name(file), fd(-1), len(0), ptr(0), ok(false), rm_file(false), mtx()
        {
            this->fd = open(this->file_name.c_str(), O_RDWR, 0664);

            if (this->fd > 0)
            {
                struct stat sb;
                if (fstat(this->fd, &sb) == 0 && S_ISREG(sb.st_mode))
                {
                    this->len = sb.st_size;
                    this->ptr = (char *)mmap(0, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
                    if (this->ptr == MAP_FAILED)
                    {
                        this->ptr = 0;
                    }
                    else
                    {
                        this->ok = true;
                    }
                }
            }
        }
        shared_file_memory(size_t len)
            : file_name(mongols::random_string("").append(".bin")), fd(-1), len(0), ptr(0), ok(false), rm_file(true), mtx()
        {
            this->fd = open(this->file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0664);
            if (this->fd > 0)
            {
                this->alloc(len);
                this->ptr = (char *)mmap(0, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
                if (this->ptr == MAP_FAILED)
                {
                    this->ptr = 0;
                }
                else
                {
                    this->ok = true;
                }
            }
        }

        virtual ~shared_file_memory()
        {
            if (this->ptr)
            {
                munmap(this->ptr, this->len);
            }
            this->ok = false;
            if (this->fd > 0)
            {
                close(fd);
            }
            if (this->rm_file)
            {
                remove(this->file_name.c_str());
            }
        }

        bool is_ok() const
        {
            return this->ok;
        }
        bool is_ok()
        {
            return this->ok;
        }

        char *get() const
        {
            return this->ptr;
        }
        char *get()
        {
            return this->ptr;
        }
        size_t size() const
        {
            return this->len;
        }
        size_t size()
        {
            return this->len;
        }

        const std::string &get_file_name() const
        {
            return this->file_name;
        }
        const std::string &get_file_name()
        {
            return this->file_name;
        }

        void lock()
        {
            this->mtx.lock();
        }

        void unlock()
        {
            this->mtx.unlock();
        }

        void run(const std::function<void(char *, size_t)> &f)
        {
            this->mtx.lock();
            f(this->ptr, this->len);
            this->mtx.unlock();
        }

        void set_rm_file(bool b)
        {
            this->rm_file = b;
        }

        void sync()
        {
            this->mtx.lock();
            msync(this->ptr, this->len, MS_SYNC);
            this->mtx.unlock();
        }

        void async()
        {
            this->mtx.lock();
            msync(this->ptr, this->len, MS_ASYNC);
            this->mtx.unlock();
        }

        bool resize(size_t len)
        {
            if (len <= this->len)
            {
                return false;
            }
            this->mtx.lock();
            char *old_ptr = (char *)mremap(this->ptr, this->len, len, MREMAP_MAYMOVE);
            if (old_ptr == MAP_FAILED)
            {
                this->mtx.unlock();
                return false;
            }
            lseek(this->fd, 0, SEEK_END);
            this->alloc(len);
            lseek(this->fd, 0, SEEK_SET);
            this->ptr = old_ptr;
            this->mtx.unlock();
            return true;
        }

    private:
        void alloc(size_t len)
        {
            size_t diff = len - this->len, count = diff / 1024, increase = 0;
            std::string tmp;
            if (count == 0)
            {
                tmp.assign(diff, '\n');
                if ((increase = write(fd, tmp.c_str(), tmp.size())) > 0)
                {
                    this->len += increase;
                }
            }
            else
            {
                tmp.assign(1024, '\n');
                for (size_t i = 0; i < count; ++i)
                {
                    if ((increase = write(fd, tmp.c_str(), tmp.size())) > 0)
                    {
                        this->len += increase;
                    }
                }
                size_t mod = diff % 1024;
                if (mod > 0)
                {
                    tmp.assign(mod, '\n');
                    if ((increase = write(fd, tmp.c_str(), tmp.size())) > 0)
                    {
                        this->len += increase;
                    }
                }
            }
        }
    };
} // namespace mongols
