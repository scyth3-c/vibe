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

#include "../abstract.hpp"
#include "parameter_proccess.h"

#include "../routes.hpp"
#include "request.hpp"
#include "../sysop/literals.h"
#include "../sockets.h"
#include "../threading/thread_pool.h"
#include "../threading/task_manager.h"
#include "fd_validate.h"
#include "httpUtils.hpp"

using std::make_shared, std::vector, std::unique_ptr;

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
    shared_ptr<Server> connection;
    shared_ptr<threading::ThreadPool> thread_pool_;

    unordered_map<int, bool> closed_fd;
    size_t threads_{3};
    std::mutex mutex_fd;

    void Process(int, const shared_ptr<std::vector<epoll_event>>& events);
    void ProcessFileDescriptor(int);

    public:

     RequestIO(const shared_ptr<RoutesMap> &,
               int &,
               int &,
               const shared_ptr<Server>&);



    void Dispatch(int _list,   const shared_ptr<std::vector<epoll_event>>& events) ;
    void SetThreads(size_t size);
    void ExecuteRoute(int client_fd, std::array<char, DEF_BUFFER_SIZE> buffer, const shared_ptr<RoutesMap> &routes) const;

    static bool TimeGuard(const unique_ptr<listen_routes> & itr);

};

#endif //IO_H
