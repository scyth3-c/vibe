#include "../../include/vibe/threading/task_manager.h"

#include <thread>

TaskManager::TaskManager(): future_map() {
    // init dispose loop
    thread_ = make_unique<thread>([this]() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock);
                if (stop_ && future_map.empty()) return;
            }

            int key = queue_.front();

            if (auto ftr = future_map.find(key); ftr != future_map.end()) {

                if (ftr->second.first.valid()) {
                    ftr->second.first.get();
                    future_map.erase(ftr);
                    std::cout << "heho" << std::endl;
                }
            }

        }
    });
}

TaskManager::~TaskManager() {
    stop_ = false;
    condition_.notify_all();
    thread_->join();
}

void TaskManager::addTask(int fd, std::shared_future<void>& task) {
   try {
    future_map.insert(make_pair(fd, std::make_pair(move(task), false)));
   }catch (std::exception &e) {
       std::cerr << e.what() << std::endl;
       task.get();
   }
}

void TaskManager::notifyOne(const int fd) {
    try {
        queue_.push(fd);
        future_map[fd].second = true;;
        condition_.notify_one();
    }catch (std::exception &e) {
     std::cerr << e.what() << std::endl;
    }
}
