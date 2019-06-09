#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

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

namespace mongols {

class shared_mutex {
public:
    shared_mutex()
        : mtx(0)
        , mtx_attr(0)
    {
        this->mtx = (pthread_mutex_t*)mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (this->mtx != MAP_FAILED) {
            this->mtx_attr = (pthread_mutexattr_t*)mmap(0, sizeof(pthread_mutexattr_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (this->mtx_attr != MAP_FAILED) {
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
    pthread_mutex_t* mtx;
    pthread_mutexattr_t* mtx_attr;
};

template <typename T, size_t len = 1>
class shared_memory {
public:
    shared_memory()
        : ok(false)
        , data(0)
        , length(len)
        , mtx()
    {
        if (this->mtx.is_ok()) {
            if (this->data == 0) {
                this->data = (T*)mmap(0, this->length * sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (this->data != MAP_FAILED) {
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

    void run(const std::function<void(T*)>& f)
    {
        this->mtx.lock();
        f(this->data);
        this->mtx.unlock();
    }

    T* get()
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
    T* data;
    size_t length;
    mongols::shared_mutex mtx;
};

}

#endif /* SHARED_MEMORY_HPP */
