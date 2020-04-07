#ifndef ID_HPP
#define ID_HPP

#include <list>
#include <mutex>
#include <queue>

namespace mongols {
class id {
public:
    id();
    virtual ~id() = default;
    void push(size_t);
    size_t poll();

private:
    static size_t data;
    static std::mutex mtx;
    static std::queue<size_t, std::list<size_t>> data_pool;
};
}

#endif // !ID_HPP
