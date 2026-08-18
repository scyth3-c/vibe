#ifndef IO_H
#define IO_H

#include <memory>
#include <shared_mutex>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <chrono>

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
 *
 *  Reading model: the event-loop thread owns every connection until its
 *  request is complete. Bytes are drained non-blocking into a per-connection
 *  buffer, so a slow client occupies only an epoll fd (bounded by
 *  Config::max_connections and the request deadlines) instead of pinning a
 *  worker thread — the slowloris cure. Only complete requests are handed to
 *  the thread pool for route execution.
 */
class RequestIO {

    public:

    private:
    shared_ptr<std::vector<epoll_event>> events;
    shared_ptr<RoutesMap>  routes;
    unique_ptr<int> file_descriptor;
    unique_ptr<int> epoll_fd;
    shared_ptr<Server> connection;

    // Live server configuration. router.configure() applies to a running
    // server (timeouts, limits...) without a restart. Note that
    // std::atomic<std::shared_ptr<T>> is ILL-FORMED: atomic<T> requires T to
    // be trivially copyable and shared_ptr is not (GCC/libstdc++ rejects it
    // with a static_assert — "is_trivially_copyable<...shared_ptr...>").
    // The snapshot is therefore guarded by a shared_mutex: readers (workers
    // and the event loop) take a shared lock and copy the shared_ptr to an
    // immutable Config; ApplyConfig() takes a unique lock to swap it.
    mutable std::shared_mutex config_mutex_;
    std::shared_ptr<const vermell::Config> config_;

    // Thread-safe copy of the live configuration snapshot.
    [[nodiscard]] std::shared_ptr<const vermell::Config> config_snapshot() const {
        std::shared_lock lock(config_mutex_);
        return config_;
    }

    shared_ptr<threading::ThreadPool> thread_pool_;

    // Open client connections (incremented on accept, decremented on close).
    // Enforced against Config::max_connections to bound connection-flood DoS.
    mutable std::atomic<size_t> active_connections_{0};
    // Fully handled client connections (any outcome). listenOne() waits on
    // this so it can stop after serving a single request without racing the
    // epoll batches.
    mutable std::atomic<size_t> handled_{0};

    // Read state of every connection that has not produced a complete
    // request yet. Only the event-loop thread reads/writes this map, so no
    // locking is needed.
    struct ConnState {
        std::string buffer;                                     // bytes received so far
        std::chrono::steady_clock::time_point start{};          // accept time
        std::chrono::steady_clock::time_point last_activity{};  // last recv
        size_t expected = 0;     // total request size once the head is known
        bool head_known = false; // the head was validated by Message::inspect
    };
    mutable std::unordered_map<int, ConnState> pending_;

    size_t threads_{[this] {
        const auto cfg = config_snapshot();
        if (cfg && cfg->threads != 0)
            return cfg->threads;
        const unsigned int cores = std::thread::hardware_concurrency();
        return static_cast<size_t>(cores == 0 ? 8 : cores);
    }()};

    // Accepts every pending connection (the listen socket is nonblocking).
    void AcceptPending() const;
    // Drains a readable client fd (event-loop thread); dispatches the request
    // to the pool when complete, rejects or keeps waiting otherwise.
    void HandleReadable(int fd) const;
    // Reaps connections that exceeded the inactivity or total read deadline.
    void SweepStale() const;
    // Hands a complete raw request to the pool; sheds the connection (503)
    // instead of blocking the event loop when the queue is full.
    void DispatchTask(int fd, std::string raw) const;
    // Dispatcher-side connection teardown with a best-effort error response.
    void Reject(int fd, int code, const char* error) const;
    // Worker entry point: owns fd exclusively (already removed from epoll).
    void ServeRequest(int fd, std::string raw) const;

    public:

     RequestIO(const shared_ptr<vector<epoll_event>>&,
               const shared_ptr<RoutesMap> &,
               int &,
               int &,
               const shared_ptr<Server>&,
               const vermell::Config &config = {});


    void Dispatch(int notice) const;
    void SetThreads(size_t size);
    // Replaces the live configuration; re-sizes the pool when threads changed.
    void ApplyConfig(const vermell::Config& config);

    // Number of client connections that have been fully handled (any
    // outcome). Used by listenOne() to stop after one served request.
    [[nodiscard]] size_t handled_connections() const noexcept { return handled_.load(); }

    static bool TimeGuard(const RoutesMap::const_iterator & itr);
    void ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes) const;
};

#endif //IO_H
