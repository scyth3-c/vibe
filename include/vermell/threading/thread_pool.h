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
    using std::make_shared, std::shared_ptr, std::packaged_task, std::future;


    class ThreadPool {

        void init();

        size_t size_;
        size_t max_queue_size_;

        queue<std::function<void()>> queue_;
        atomic<bool> stop_;

        mutex mutex_;
        condition_variable cond_not_empty_;
        condition_variable cond_not_full_;
        vector<thread> threads_;


    public:

       explicit ThreadPool(size_t threads, size_t max_queue_size = 0);
        ~ThreadPool();

        // Blocks when the queue is full: that is the backpressure signal.
        future<void> addTask(std::function<void()> task);
        void kill();

    };

}

#endif //THREAD_POOL_H
