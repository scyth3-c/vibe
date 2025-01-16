#include "../../include/vibe/threading/thread_pool.h"

#include <iostream>
using namespace threading;

ThreadPool::ThreadPool(const size_t threads ) : size_(threads), stop_(false) {
    init();
}

ThreadPool::~ThreadPool() {
  kill();
}


future<void> ThreadPool::addFutureTask(const std::function<void(shared_ptr<promise<void>>)>& task) {

  auto future_capsule = make_shared<std::promise<void>>();
  {
    std::unique_lock<mutex> lock(mutex_);
    if(stop_.load())
      throw std::runtime_error("ThreadPool::addTask: stopped");

    queue_.emplace([future_capsule, task]() {
      try {
        task(future_capsule);
      }catch (...) {
        future_capsule->set_exception(std::current_exception());
      }
    });
  }
  condition_.notify_one();
  return future_capsule->get_future();
}


void ThreadPool::addTask(const std::function<void()>&task) {

  std::unique_lock<mutex> lock(mutex_);

  if(stop_.load())
    throw std::runtime_error("ThreadPool::addTask: stopped");

  queue_.emplace(task);
  condition_.notify_one();

}

void ThreadPool::init() {

  if(!threads_.empty())
      return;

  for (size_t i = 0; i < size_; i++) {
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
         }catch (std::exception &e) {
            std::cout << e.what() << std::endl;
         }
       }
       }
    });
  }
}


void ThreadPool::kill() {

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

