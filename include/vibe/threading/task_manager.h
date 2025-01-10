//
// Created by owl on 20/10/24.
//

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <queue>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <vector>

using std::queue, std::future,
      std::mutex, std::condition_variable,
      std::thread, std::unique_ptr, std::make_unique,
      std::unordered_map, std::vector;

class TaskManager {

    condition_variable condition_;
    mutex manage_lock;

    unordered_map<int, future<void>> inserted_fd;
    vector<int> ready_fds;

    unique_ptr<thread> thread_;

    bool stop_{};
public:

    TaskManager();
    ~TaskManager();

    void addTask(int, future<void> task);
    void addReadyFd(int fd);
};


#endif //TASK_MANAGER_H
