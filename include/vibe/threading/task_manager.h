//
// Created by owl on 20/10/24.
//

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <queue>
#include <future>
#include <memory>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <atomic>

using std::queue, std::future,
      std::mutex, std::condition_variable,
      std::thread, std::unique_ptr, std::make_unique,
      std::unordered_map;

class TaskManager {

    condition_variable condition_;
    mutex mutex_;

    unordered_map<int, std::pair<std::shared_future<void>, bool>> future_map;
    queue<int> queue_;
    unique_ptr<thread> thread_;
    bool stop_{};


public:

    TaskManager();
    ~TaskManager();

    void addTask(int, std::shared_future<void>& task);
    void notifyOne(int);
};


#endif //TASK_MANAGER_H
