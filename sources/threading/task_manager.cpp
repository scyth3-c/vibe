#include "../../include/vibe/threading/task_manager.h"

TaskManager::TaskManager(const shared_ptr<FdValidate>& fv): fd_validate_(fv){

    thread_ = make_unique<thread>([this]() {
        while (stop_ != true) {
            {
                std::unique_lock<mutex> lock(manage_lock);
                condition_.wait(lock);
                if (stop_ && futures_.empty()) return;
            }

            if (stop_ && futures_.empty()) break;
            if(futures_.empty()) continue;

            std::unique_lock<mutex> lock(manage_lock);
            for(auto it = futures_.begin(); it != futures_.end();) {
                if(it->second.first.valid() && it->second.second) {

                    it->second.first.get();

                    if(futures_.find(it->first) != futures_.end()) {
                        it = futures_.erase(it);
                    }
                   }else {
                    ++it;
                }
            }
            lock.unlock();

            fd_validate_->clearOldFd();
            fd_validate_->retryBusyMessages();
        }
    });
}

TaskManager::~TaskManager() {
    kill();
}

void TaskManager::kill() {
    stop_ = true;
    condition_.notify_all();
    if(thread_->joinable()) thread_->join();
    terminal("TaskManager::kill");
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



