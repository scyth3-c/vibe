#include "../../include/vibe/threading/task_manager.h"

#include <thread>

#include "../../include/vibe/util/nterminal.h"

TaskManager::TaskManager(const std::shared_ptr<FdValidate>& fv): fd_validate_(fv){

    // init dispose loop
    thread_ = make_unique<thread>([this]() {
        while (true) {
            {
                std::unique_lock<mutex> lock(manage_lock);
                condition_.wait(lock);
                if (stop_ && futures_.empty()) return;
            }

            if (stop_ && futures_.empty()) break;
            if(futures_.empty()) continue;

            std::cout << "popping " << futures_.size() << std::endl;

            for(auto it = futures_.begin(); it != futures_.end();) {
                if(it->second.valid()) {
                    it->second.get();
                    it = futures_.erase(it);
                }else {
                    ++it;
                }
            }

        }
    });
}

TaskManager::~TaskManager() {
    stop_ = false;
    condition_.notify_all();

    for (const auto& [fd, _] : futures_) {
        fd_validate_->free(fd);
    }

    if(thread_->joinable()) thread_->join();
}

void TaskManager::manage(const int fd_i, future<void> task)
{
       std::lock_guard<mutex> lock(manage_lock);
       futures_.emplace(fd_i,std::move(task));
}

void TaskManager::dispose()
{
    std::unique_lock<mutex> lock(manage_lock);
    condition_.notify_all();
}



