#include "../../include/vibe/threading/task_manager.h"


TaskManager::TaskManager(const shared_ptr<FdValidate>& fv): fd_validate_(fv){

    thread_ = make_unique<thread>([this]() {
        while (true) {
            {
                std::unique_lock<mutex> lock(manage_lock);
                condition_.wait(lock);
                if (stop_ && futures_.empty()) return;
            }

            if (stop_ && futures_.empty()) break;
            if(futures_.empty()) continue;

            for(auto it = futures_.begin(); it != futures_.end();) {

                if(it->second.first.valid() && it->second.second) {

                    it->second.first.get();
                    it = futures_.erase(it);

                }else {
                    ++it;
                }
            }

            fd_validate_->clearOldFd();

        }
    });
}

TaskManager::~TaskManager() {
    stop_ = true;
    condition_.notify_all();

    if(thread_->joinable()) thread_->join();
}

void TaskManager::manage(const std::string& key, future<void> task)
{
       std::lock_guard<mutex> lock(manage_lock);
       futures_.emplace(key, std::make_pair(std::move(task), false));
}

void TaskManager::releaseOne(const string& key)
{
    std::lock_guard<mutex> lock(manage_lock);
    if (futures_.find(key) == futures_.end()) return;

    futures_[key].second = true;
    condition_.notify_all();

}

void TaskManager::dispose()
{
    std::unique_lock<mutex> lock(manage_lock);
    condition_.notify_all();
}



