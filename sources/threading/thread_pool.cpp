#include "../../include/vibe/threading/thread_pool.h"

#include <iostream>
using namespace threading;

ThreadPool::ThreadPool(size_t threads, size_t max_queue_size)
    : size_(threads), max_queue_size_(max_queue_size) {
    init();
}

ThreadPool::~ThreadPool() {
    kill();
}

std::future<void> ThreadPool::addFutureTask(const std::function<void(std::shared_ptr<std::promise<void>>)> &task) {
    auto promise_ptr = std::make_shared<std::promise<void>>();
    std::future<void> fut = promise_ptr->get_future();

    std::unique_lock<std::mutex> lock(mutex_);
    // Espera hasta que haya espacio en la cola
    cond_not_full_.wait(lock, [this] {
        return stop_ || queue_.size() < max_queue_size_;
    });
    if (stop_) {
        throw std::runtime_error("ThreadPool::addFutureTask: stopped");
    }

    // Encapsular la tarea
    queue_.emplace([promise_ptr, task] {
        try {
            task(promise_ptr);
        } catch (...) {
            promise_ptr->set_exception(std::current_exception());
        }
    });

    // Notificar al hilo trabajador
    cond_not_empty_.notify_one();
    return fut;
}

void ThreadPool::addTask(const std::function<void()> &task) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_not_full_.wait(lock, [this] {
        return stop_ || queue_.size() < max_queue_size_;
    });
    if (stop_) {
        throw std::runtime_error("ThreadPool::addTask: stopped");
    }

    queue_.emplace(task);
    cond_not_empty_.notify_one();
}

void ThreadPool::init() {
    if (!threads_.empty()) return;

    for (size_t i = 0; i < size_; ++i) {
        threads_.emplace_back([this] {
            while (true) {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_not_empty_.wait(lock, [this] {
                        return stop_ || !queue_.empty();
                    });
                    if (stop_ && queue_.empty()) {
                        return;
                    }
                    job = std::move(queue_.front());
                    queue_.pop();
                    // Avisar a productores de espacio libre
                    cond_not_full_.notify_one();
                }
                if (job) job();
            }
        });
    }
}

void ThreadPool::kill() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_not_empty_.notify_all();
    cond_not_full_.notify_all();
    for (auto &t : threads_) {
        if (t.joinable()) t.join();
    }
}

// namespace threading
