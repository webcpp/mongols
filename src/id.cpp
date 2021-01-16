#include "id.hpp"

namespace mongols
{
    size_t id::data = 0;
    std::mutex id::mtx;
    std::queue<size_t, std::list<size_t>> id::data_pool;

    size_t id::poll()
    {
        std::lock_guard<std::mutex> guard(id::mtx);
        if (id::data_pool.empty())
        {
            ++id::data;
            return id::data;
        }
        size_t tmp = id::data_pool.front();
        id::data_pool.pop();
        return tmp;
    }

    void id::push(size_t t)
    {
        std::lock_guard<std::mutex> guard(id::mtx);
        id::data_pool.push(t);
    }

} // namespace mongols