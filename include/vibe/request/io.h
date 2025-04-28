//
// Created by owl on 19/08/24.
//

#ifndef IO_H
#define IO_H

#include <memory>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unordered_map>
#include <vector>
#include <functional>

#include "../abstract.hpp"
#include "parameter_proccess.h"

#include "../request/routes.hpp"
#include "request.hpp"
#include "../sysop/literals.h"
#include "../sysop/sysprocess.h"
#include "../sockets.h"
#include "../threading/thread_pool.h"
#include "../threading/task_manager.h"
#include "fd_validate.h"
#include "../request/httpUtils.hpp"
#include "../configuration.hpp"

using std::make_shared, std::vector, std::unique_ptr, std::function;

using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;
/*
 *  RequestIO for Server socket class, if you want implement other Server, you should create other RequestIO for the implementation
 */
class RequestIO {

    unique_ptr<TaskManager> taskManager;
    unique_ptr<int> file_descriptor;
    unique_ptr<int> epoll_fd;

    shared_ptr<FdValidate> fd_validate;
    shared_ptr<RoutesMap>  routes;
    shared_ptr<threading::ThreadPool> thread_pool_;

    unordered_map<int, bool> closed_fd;
    size_t threads_{THREADS};
    std::mutex mutex_fd;
    neo::LISTEN_TYPE listen_type;
    std::function<void()> step_process{};
    std::atomic<int>  general_increment{};
    std::function<void()> parent_callback = nullptr;

    void Process(int,const string&,const shared_ptr<std::vector<epoll_event>>& events);
    bool ProcessFileDescriptor(int,const string&);

    public:

     RequestIO(const shared_ptr<RoutesMap> &,
               int &,
               int &,
               neo::LISTEN_TYPE,
               const std::function<void()> &);

    void Dispatch(int _list,   const shared_ptr<std::vector<epoll_event>>& events) ;
    void SetThreads(size_t size);
    bool ExecuteRoute(int client_fd, const string& key, const std::array<char, DEF_BUFFER_SIZE> &buffer, const shared_ptr<RoutesMap> &routes) const;

    static bool TimeGuard(const unique_ptr<listen_routes> & itr);

};

#endif //IO_H
