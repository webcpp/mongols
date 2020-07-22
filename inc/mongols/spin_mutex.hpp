#ifndef E08DF8D4_7BA1_4CBA_AE5F_4DFA7DA56D3D
#define E08DF8D4_7BA1_4CBA_AE5F_4DFA7DA56D3D

#include <atomic>

namespace mongols {

class spin_mutex {
private:
    std::atomic_flag flag;

public:
    spin_mutex()
        : flag(ATOMIC_FLAG_INIT)
    {
    }
    virtual ~spin_mutex() = default;

    void lock()
    {
        while (this->flag.test_and_set(std::memory_order_acquire))
            ;
    }

    void unlock()
    {
        this->flag.clear(std::memory_order_release);
    }
};
}

#endif /* E08DF8D4_7BA1_4CBA_AE5F_4DFA7DA56D3D */
