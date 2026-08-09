#include "../../include/vibe/threading/thread_pool.h"

#include <iostream>

using namespace threading;

ThreadPool::ThreadPool(const size_t threads) : size_(threads == 0 ? 1 : threads), stop_(false) {

  for (size_t i = 0; i < size_; i++) {

    threads_.emplace_back([this]() {

       while(true){

         std::function<void()> task;

         {
               std::unique_lock<std::mutex> lock(mutex_);
               this->condition_.wait(lock, [this] { return this->stop_.load() || !this->queue_.empty();});

               if(this->stop_.load() && this->queue_.empty())
                 return;

               task = std::move(this->queue_.front());
               this->queue_.pop();
         }

         // A task must never escape an exception: it would call std::terminate
         // and kill the whole process.
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

ThreadPool::~ThreadPool() {

    stop_.store(true);
    condition_.notify_all();
    for(thread &thread : threads_)
      if(thread.joinable())
        thread.join();
}


future<void> ThreadPool::addTask(std::function<void()> task) {

  const auto task_ptr = std::make_shared<packaged_task<void()>>(std::move(task));

  std::future<void> future = task_ptr->get_future();

  {
    std::unique_lock<std::mutex> lock(mutex_);
    if(stop_.load())
      throw std::runtime_error("ThreadPool::addTask: stopped");

    queue_.emplace([task_ptr]() { (*task_ptr)(); });
  }

  condition_.notify_one();
  return future;
}
