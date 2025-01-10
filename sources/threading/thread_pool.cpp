#include "../../include/vibe/threading/thread_pool.h"

#include <iostream>
using namespace threading;

ThreadPool::ThreadPool(const size_t threads ) : size_(threads), stop_(false) {

  for (size_t i = 0; i < threads; i++) {

    threads_.emplace_back([this]() {

       while(!stop_){

         std::function<void()> task;

         {
               std::unique_lock<std::mutex> lock(mutex_);
               this->condition_.wait(lock, [this] { return this->stop_ || !this->queue_.empty();});

               if(this->stop_)
                 return;

              if (!this->queue_.empty())
              {
                task = std::move(this->queue_.front());
                if (task == nullptr)
                  continue;
                queue_.pop();
              }
         }

       if(task)
       {
         try
         {
           task();
         }catch (std::exception &e)
         {
           std::cerr << e.what() << std::endl;
         }
       }
       }
    });
  }
}

ThreadPool::~ThreadPool() {

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }

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