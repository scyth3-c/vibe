#include "../../include/vibe/threading/task_manager.h"

#include <thread>

TaskManager::TaskManager() {
    // init dispose loop
    thread_ = make_unique<thread>([this]() {
        while (true) {

            {
                std::unique_lock<mutex> lock(manage_lock);
                condition_.wait(lock);
                if (stop_ && inserted_fd.empty()) return;
            }

            std::cout << "popping " << inserted_fd.size() << std::endl;

            if (stop_ && inserted_fd.empty()) break;

            for (auto it = ready_fds.begin(); it != ready_fds.end();) {

                if (auto fd_it = inserted_fd.find(*it); fd_it != inserted_fd.end()) {

                    try {

                        auto& future = fd_it->second;
                        if (future.valid()) {

                            future.wait();
                            future.get();
                        }
                        inserted_fd.erase(fd_it);
                        it = ready_fds.erase(it);
                    } catch (const std::exception& e) {

                        ++it;
                    }
                } else {
                    ++it;
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

void TaskManager::addTask(const int fd, future<void> task)
{
       std::lock_guard<mutex> lock(manage_lock);
       inserted_fd[fd] = std::move(task);
}

void TaskManager::addReadyFd(const int fd)
{
        std::lock_guard<mutex> lock(manage_lock);
        ready_fds.push_back(fd);
        condition_.notify_all();
}
