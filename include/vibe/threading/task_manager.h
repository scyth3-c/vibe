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

using std::future, std::string,
      std::mutex, std::condition_variable,
      std::thread, std::unique_ptr, std::make_unique,
      std::unordered_map, std::vector, std::pair, std::shared_ptr;

class TaskManager {

    condition_variable condition_;
    mutex manage_lock;

    unordered_map<string, pair<future<void>, bool> > futures_;
    unique_ptr<thread> thread_;
    shared_ptr<FdValidate> fd_validate_;

    bool stop_{false};
public:

    explicit TaskManager(const shared_ptr<FdValidate>&);
    ~TaskManager();

    void manage(const string& key, future<void> task);
    void releaseOne(const string& key);

    void dispose();

};


#endif //TASK_MANAGER_H
