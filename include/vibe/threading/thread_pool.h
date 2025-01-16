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
#include "../util/enums.h"


namespace threading {

    using std::vector, std::mutex, std::condition_variable, std::atomic, std::queue, std::thread;
    using std::make_shared, std::shared_ptr, std::packaged_task, std::future, std::promise, std::function;

    class ThreadPool {

        vector<thread> threads_;
        size_t size_;

        queue<function<void()>> queue_;
        atomic<bool> stop_;

        mutex mutex_;
        condition_variable condition_;

    public:

       explicit ThreadPool(size_t threads = 4);
        ~ThreadPool();

        future<void> addFutureTask(const std::function<void(shared_ptr<promise<void>>)>& task);
        void addTask(const std::function<void()>&task);

        void init();
        void kill();

    };


}

#endif //THREAD_POOL_H
