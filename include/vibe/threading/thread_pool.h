#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <memory>
#include "../abstract.hpp"
#include "../configuration.hpp"


namespace threading {

    using std::vector, std::mutex, std::condition_variable, std::atomic, std::queue, std::thread;
    using std::make_shared, std::shared_ptr, std::packaged_task, std::future, std::promise, std::function;

    class ThreadPool {

        void init();

        size_t size_;
        size_t max_queue_size_;
        std::atomic<bool> stop_{false};

        std::mutex mutex_;
        std::condition_variable cond_not_empty_;
        std::condition_variable cond_not_full_;
        std::queue<std::function<void()>> queue_;
        std::vector<std::thread> threads_;

    public:

        ThreadPool(size_t threads, size_t max_queue_size);
        ~ThreadPool();
        std::future<void> addFutureTask(const std::function<void(std::shared_ptr<std::promise<void>>)> &task);
        void addTask(const std::function<void()> &task);
        void kill();

    };


}

#endif //THREAD_POOL_H
