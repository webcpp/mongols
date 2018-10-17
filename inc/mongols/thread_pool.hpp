#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <functional>
#include <vector>
#include <atomic>
#include <iostream>

#include "safe_queue.hpp"


namespace mongols {

    class join_thread {
    private:
        std::vector<std::thread>& th;
    public:

        join_thread(std::vector<std::thread>& th) : th(th) {
        }

        virtual~join_thread() {
            for (auto& i : this->th) {
                if (i.joinable()) {
                    i.join();
                }
            }
        }

    };

    template<typename function_t>
    class thread_pool {
    private:
        safe_queue<function_t > q;
        std::vector<std::thread> th;
        join_thread joiner;
        std::atomic_bool done;

        void work() {
            function_t task;
            while (this->done) {
                this->q.wait_and_pop(task);
                task();
            }
        }

        void shutdown() {
            this->done = false;
            for (size_t i = 0; i < this->th.size(); ++i) {
                this->submit([]() {
                    return true;
                });
            }
        }
    public:

        thread_pool(size_t th_size = std::thread::hardware_concurrency()) : q(), th(), joiner(th), done(true) {
            try {
                for (size_t i = 0; i < th_size; ++i) {
                    this->th.push_back(std::move(std::thread(std::bind(&thread_pool::work, this))));
                }
            } catch (...) {
                this->shutdown();
            }
        }

        virtual~thread_pool() {
            this->shutdown();
        }

        void submit(function_t&& f) {
            if (!this->th.empty()) {
                this->q.push(std::move(f));
            }
        }

        size_t size()const {
            return this->th.size();
        }

        bool empty()const {
            return this->th.empty();
        }

    };
}

#endif /* THREAD_POOL_HPP */

