//
// Created by owl on 19/08/24.
//

#ifndef IO_H
#define IO_H

#include <memory>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unordered_map>
#include <atomic>
#include <thread>

#include "../util/enums.h"
#include "../util/parameter_proccess.h"

#include "../config.hpp"
#include "../routes.hpp"
#include "request.hpp"
#include "../util/nterminal.h"
#include "../sockets.h"
#include "../threading/thread_pool.h"

using std::make_shared, std::vector, std::unique_ptr;

using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;
/*
 *  RequestIO for Server socket class, if you want implement other Server, you should create other RequestIO for the implementation
 */
class RequestIO {

    public:

     enum class ReadStatus { Ok, TooLarge, Failed };

    private:
    shared_ptr<std::vector<epoll_event>> events;
    shared_ptr<RoutesMap>  routes;
    unique_ptr<int> file_descriptor;
    unique_ptr<int> epoll_fd;
    shared_ptr<Server> connection;

    vibe::Config config_{};

    shared_ptr<threading::ThreadPool> thread_pool_;

    size_t threads_{[this] {
        if (config_.threads != 0)
            return config_.threads;
        const unsigned int cores = std::thread::hardware_concurrency();
        return static_cast<size_t>(cores == 0 ? 8 : cores);
    }()};

    // Accepts every pending connection (the listen socket is nonblocking).
    void AcceptPending() const;
    // Worker entry point: owns event_fd exclusively (already removed from epoll).
    void HandleClient(int event_fd) const;
    // Reads a complete HTTP request from a nonblocking fd.
    ReadStatus ReadRequest(int event_fd, string &out) const;

    public:

     RequestIO(const shared_ptr<vector<epoll_event>>&,
               const shared_ptr<RoutesMap> &,
               int &,
               int &,
               const shared_ptr<Server>&,
               const vibe::Config &config = {});


    void Dispatch(int notice) const;
    void SetThreads(size_t size);

    static bool TimeGuard(const RoutesMap::const_iterator & itr);
    static void ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes);
};

#endif //IO_H
