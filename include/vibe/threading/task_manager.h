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
#include "../request/fd_validate.h"
#include <iostream>
#include <unistd.h>

using std::queue, std::future,
      std::mutex, std::condition_variable,
      std::thread, std::unique_ptr, std::make_unique,
      std::unordered_map, std::vector;

class TaskManager {

    condition_variable condition_;
    mutex manage_lock;

    unordered_map<int, future<void>> futures_;
    unique_ptr<thread> thread_;
    std::shared_ptr<FdValidate> fd_validate_;

    bool stop_{};
public:

    explicit TaskManager(const std::shared_ptr<FdValidate>&);
    ~TaskManager();

    void manage(int fd_i, future<void> task);
    void dispose();

};


#endif //TASK_MANAGER_H
