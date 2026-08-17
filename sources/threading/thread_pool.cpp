#include "../../include/vermell/threading/thread_pool.h"

#include <algorithm>
#include <iostream>

namespace {

    // Default capacity keeps the epoll loop fed without turning the queue into
    // an unbounded memory sink: at least 1024 tasks, or 256 per worker.
    constexpr size_t MIN_QUEUE_CAPACITY = 1024;
    constexpr size_t QUEUE_CAPACITY_PER_THREAD = 256;

    size_t queue_capacity(const size_t threads, const size_t configured) {
        if (configured != 0)
            return configured;
        return std::max(threads * QUEUE_CAPACITY_PER_THREAD, MIN_QUEUE_CAPACITY);
    }
}

using namespace threading;

ThreadPool::ThreadPool(const size_t threads, const size_t max_queue_size)
    : size_(threads == 0 ? 1 : threads),
      max_queue_size_(queue_capacity(size_, max_queue_size)),
      stop_(false) {
    init();
}

ThreadPool::~ThreadPool() {
    kill();
}


void ThreadPool::init() {

    if (!threads_.empty())
        return;

    for (size_t i = 0; i < size_; i++) {

        threads_.emplace_back([this]() {

            while (true) {

                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    this->cond_not_empty_.wait(lock, [this] {
                        return this->stop_.load() || !this->queue_.empty();
                    });

                    if (this->stop_.load() && this->queue_.empty())
                        return;

                    task = std::move(this->queue_.front());
                    this->queue_.pop();
                    this->cond_not_full_.notify_one();
                }

                // A task must never escape an exception: it would call
                // std::terminate and kill the whole process.
                try {
                    task();
                } catch (const std::exception &e) {
                    std::cerr << "ThreadPool: task exception: " << e.what() << '\n';
                } catch (...) {
                    std::cerr << "ThreadPool: unknown task exception\n";
                }
            }
        });
    }
}


future<void> ThreadPool::addTask(std::function<void()> task) {

    const auto task_ptr = std::make_shared<packaged_task<void()>>(std::move(task));

    std::future<void> future = task_ptr->get_future();

    {
        std::unique_lock<std::mutex> lock(mutex_);
        this->cond_not_full_.wait(lock, [this] {
            return this->stop_.load() || this->queue_.size() < this->max_queue_size_;
        });

        if (this->stop_.load())
            throw std::runtime_error("ThreadPool::addTask: stopped");

        this->queue_.emplace([task_ptr]() { (*task_ptr)(); });
    }

    this->cond_not_empty_.notify_one();
    return future;
}


bool ThreadPool::tryAddTask(std::function<void()> task) {

    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (this->stop_.load())
            return false;
        if (this->queue_.size() >= this->max_queue_size_)
            return false;
        this->queue_.emplace(std::move(task));
    }

    this->cond_not_empty_.notify_one();
    return true;
}


void ThreadPool::kill() {

    {
        std::unique_lock<std::mutex> lock(mutex_);
        this->stop_.store(true);
    }

    this->cond_not_empty_.notify_all();
    this->cond_not_full_.notify_all();

    for (thread &worker : this->threads_) {
        if (worker.joinable())
            worker.join();
    }
}
