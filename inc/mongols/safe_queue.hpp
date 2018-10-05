#ifndef SAFE_QUEUE_HPP
#define SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>


namespace mongols {

    template<typename T>
    class safe_queue {
    private:
        mutable std::mutex mtx;
        std::queue<T> q;
        std::condition_variable cv;
    public:

        safe_queue() : mtx(), q(), cv() {

        }
        virtual~safe_queue() = default;

        void push(const T& v) {
            std::lock_guard<std::mutex> lk(this->mtx);
            this->q.push(v);
            this->cv.notify_one();
        }

        void push(T&& v) {
            std::lock_guard<std::mutex> lk(this->mtx);
            this->q.push(std::move(v));
            this->cv.notify_one();
        }

        void wait_and_pop(T& v) {
            std::unique_lock<std::mutex> lk(this->mtx);
            this->cv.wait(lk, [&]() {
                return !this->q.empty();
            });
            v = std::move(this->q.front());
            this->q.pop();
        }

        bool try_pop(T& v) {
            std::lock_guard<std::mutex> lk(this->mtx);
            if (this->q.empty()) {
                return false;
            }
            v = std::move(this->q.front());
            this->q.pop();
            return true;
        }

        bool empty()const {
            std::lock_guard<std::mutex> lk(this->mtx);
            return this->q.empty();
        }

    };
}

#endif /* SAFE_QUEUE_HPP */

