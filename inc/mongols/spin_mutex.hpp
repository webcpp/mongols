#ifndef SPIN_MUTEX_HPP
#define SPIN_MUTEX_HPP

#include <atomic>

namespace mongols {

    class spin_mutex {
    private:
        std::atomic_flag flag;
    public:

        spin_mutex() : flag(ATOMIC_FLAG_INIT) {
        }
        virtual~spin_mutex() = default;

        void lock() {
            while (this->flag.test_and_set(std::memory_order_acquire));
        }

        void unlock() {
            this->flag.clear(std::memory_order_release);
        }
    };
}

#endif /* SPIN_MUTEX_HPP */

