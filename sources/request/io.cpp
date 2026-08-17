#include "../../include/vermell/request/io.h"

#include <cerrno>
#include <cstring>

namespace {

    // Decrements the connection counter when a worker is done with a fd, on
    // every exit path of ServeRequest (including early returns).
    struct ConnectionGuard {
        std::atomic<size_t>& counter;
        explicit ConnectionGuard(std::atomic<size_t>& c) : counter(c) {}
        ~ConnectionGuard() { counter.fetch_sub(1); }
    };

    // Single-shot, non-blocking write for dispatcher-generated errors: the
    // event loop must never wait on a stuck client, so the response is sent
    // once and dropped on EAGAIN (the connection is closed right after).
    void send_best_effort(const int fd, const std::string& msg) {
        (void)::send(fd, msg.data(), msg.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
    }

    // Minimal JSON error response (same shape the worker paths produce).
    std::string error_response(const int code, const char* error) {
        return vermell::http::Response{}
                   .status(code)
                   .type("application/json")
                   .body(std::string(R"lit({"error":")lit") + error + R"lit("})lit")
                   .str();
    }

} // namespace

RequestIO::RequestIO(const shared_ptr<vector<epoll_event> > &events,
                     const std::shared_ptr<RoutesMap> &routes,
                     int &filed,
                     int &epoll_fd,
                     const shared_ptr<Server> &con,
                     const vermell::Config &config) : events(events),
                                                   routes(routes),
                                                   file_descriptor(std::make_unique<int>(filed)),
                                                   epoll_fd(std::make_unique<int>(epoll_fd)),
                                                   connection(con),
                                                   config_(std::make_shared<const vermell::Config>(config)) {

    thread_pool_ = make_shared<threading::ThreadPool>(threads_, config_.load()->max_queue_size);
}


void RequestIO::Dispatch(const int notice) const {

    if (notice <= 0) {
        SweepStale(); // idle wake-up: reap connections that ran out of time
        return;
    }

    for (int i = 0; i < notice; i++) {

        const int event_fd = events->operator[](i).data.fd;
        const uint32_t event_mask = events->operator[](i).events;

        if (event_fd == *file_descriptor) {
            AcceptPending();
            continue;
        }

        if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
            // The peer closed or the fd errored. If readable data is still
            // pending, drain it first (a client may half-close after sending
            // a complete request); anything still incomplete is dropped.
            if (event_mask & EPOLLIN)
                HandleReadable(event_fd);
            if (pending_.contains(event_fd)) {
                epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);
                close(event_fd);
                pending_.erase(event_fd);
                active_connections_.fetch_sub(1);
                handled_.fetch_add(1);
            }
            continue;
        }

        if (event_mask & EPOLLIN)
            HandleReadable(event_fd);
    }

    SweepStale();
}


void RequestIO::AcceptPending() const {

    for (;;) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        const int client_file_descriptor = accept(*file_descriptor,
                                                  reinterpret_cast<sockaddr *>(&client_addr),
                                                  &client_addr_len);

        if (client_file_descriptor == VER_NVALUE) {
            if (errno == EINTR)
                continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                terminal(VER_EPOLL_CERR, strerror(errno));
            return;
        }

        // Shedding: when the connection cap is reached, accept and close
        // immediately so the listen backlog drains (the client sees a reset)
        // instead of spinning the event loop with an undrained readable fd.
        if (config_.load()->max_connections != 0
            && active_connections_.load() >= config_.load()->max_connections) {
            close(client_file_descriptor);
            continue;
        }

        if (Server::setNonblocking(client_file_descriptor) == VER_SOCKET_ERROR) {
            close(client_file_descriptor);
            continue;
        }

        epoll_event client_event{};
        client_event.events = EPOLLIN; // level triggered; kept registered while reading
        client_event.data.fd = client_file_descriptor;

        if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, client_file_descriptor, &client_event) == VER_NVALUE) {
            terminal(VER_EPOLL_CERR, strerror(errno));
            close(client_file_descriptor);
            continue;
        }

        active_connections_.fetch_add(1);

        // Track the connection from birth: the deadlines below apply even to
        // clients that connect and never send a byte.
        const auto now = std::chrono::steady_clock::now();
        auto& st = pending_[client_file_descriptor];
        st.start = now;
        st.last_activity = now;
    }
}


void RequestIO::HandleReadable(const int fd) const {

    const auto cfg = config_.load();

    auto& st = pending_[fd];
    const auto now = std::chrono::steady_clock::now();
    if (st.start == std::chrono::steady_clock::time_point{})
        st.start = now;

    // ---- drain everything the socket has right now (non-blocking) ----
    std::vector<char> chunk(cfg->read_chunk);
    bool peer_closed = false;
    for (;;) {
        const ssize_t bytes = recv(fd, chunk.data(), chunk.size(), 0);
        if (bytes > 0) {
            st.buffer.append(chunk.data(), static_cast<size_t>(bytes));
            st.last_activity = std::chrono::steady_clock::now();
            if (st.buffer.size() > cfg->max_request_size) {
                Reject(fd, 413, "payload too large");
                return;
            }
            continue; // keep draining until the socket reports EAGAIN
        }
        if (bytes == 0) {
            peer_closed = true; // FIN: no more bytes will ever come
            break;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break; // drained: wait for the next EPOLLIN
        Reject(fd, 400, "malformed request");
        return;
    }

    if (st.buffer.empty()) {
        // No data at all; the deadlines in SweepStale will reap the client.
        return;
    }

    const bool deadline_over =
        (cfg->request_timeout.count() > 0 && now - st.start > cfg->request_timeout) ||
        (cfg->read_timeout.count() > 0 && now - st.last_activity > cfg->read_timeout);

    // ---- head known: only wait for the promised body bytes ----
    if (st.head_known) {
        if (st.buffer.size() >= st.expected) {
            epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, nullptr); // worker owns the fd now
            std::string raw = std::move(st.buffer);
            pending_.erase(fd);
            DispatchTask(fd, std::move(raw));
            return;
        }
        if (peer_closed) { // promised bytes never arrived
            Reject(fd, 400, "malformed request");
            return;
        }
        if (deadline_over) {
            Reject(fd, 408, "request timeout");
            return;
        }
        return; // body still on the wire: stay registered in epoll
    }

    // ---- head phase: ask the parser for the framing verdict ----
    const auto inspection = vermell::http::Message::inspect(st.buffer);
    switch (inspection.framing) {
        case vermell::http::Message::Framing::Incomplete: {
            if (inspection.expected > cfg->max_request_size) {
                Reject(fd, 413, "payload too large");
                return;
            }
            st.expected = inspection.expected;
            st.head_known = inspection.expected > 0;
            if (peer_closed) {
                Reject(fd, 400, "malformed request");
                return;
            }
            if (deadline_over) {
                Reject(fd, 408, "request timeout");
                return;
            }
            return; // keep reading
        }
        case vermell::http::Message::Framing::Complete: {
            epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, nullptr); // worker owns the fd now
            std::string raw = std::move(st.buffer);
            pending_.erase(fd);
            DispatchTask(fd, std::move(raw));
            return;
        }
        case vermell::http::Message::Framing::BadRequest:
            Reject(fd, 400, "malformed request");
            return;
        case vermell::http::Message::Framing::TooManyHeaders:
            Reject(fd, 431, "too many headers");
            return;
        case vermell::http::Message::Framing::NotImplemented:
            Reject(fd, 501, "transfer encoding not supported");
            return;
    }
}


void RequestIO::SweepStale() const {

    const auto cfg = config_.load();
    if (cfg->request_timeout.count() <= 0 && cfg->read_timeout.count() <= 0)
        return; // deadlines disabled

    const auto now = std::chrono::steady_clock::now();
    for (auto it = pending_.begin(); it != pending_.end();) {
        const bool total_over = cfg->request_timeout.count() > 0
                                && now - it->second.start > cfg->request_timeout;
        const bool idle_over = cfg->read_timeout.count() > 0
                               && now - it->second.last_activity > cfg->read_timeout;
        if (!total_over && !idle_over) {
            ++it;
            continue;
        }

        const int fd = it->first;
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        send_best_effort(fd, error_response(408, "request timeout"));
        close(fd);
        active_connections_.fetch_sub(1);
        handled_.fetch_add(1);
        it = pending_.erase(it);
    }
}


void RequestIO::DispatchTask(const int fd, std::string raw) const {

    bool accepted = false;
    try {
        accepted = thread_pool_->tryAddTask([this, fd, raw = std::move(raw)]() mutable {
            this->ServeRequest(fd, std::move(raw));
        });
    } catch (const std::exception &e) {
        terminal("THREAD POOL REJECTED TASK: ", e.what());
    }

    if (accepted)
        return; // the worker owns the fd and closes it after responding

    // Queue full: shed instead of blocking the event loop (backpressure).
    send_best_effort(fd, error_response(503, "server busy"));
    close(fd);
    active_connections_.fetch_sub(1);
    handled_.fetch_add(1);
}


void RequestIO::Reject(const int fd, const int code, const char* error) const {
    epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    send_best_effort(fd, error_response(code, error));
    close(fd);
    pending_.erase(fd);
    active_connections_.fetch_sub(1);
    handled_.fetch_add(1);
}


void RequestIO::ServeRequest(const int fd, std::string raw) const {

    // This fd was accepted (counted) and now belongs exclusively to this
    // worker; the guard releases the count on every exit path.
    ConnectionGuard guard(active_connections_);

    const auto base = std::make_shared<Server>();

    base->setPort(connection->getPort());
    base->setSocketId(fd);
    base->setWriteTimeout(config_.load()->write_timeout);
    base->setResponse(std::move(raw));

    ExecuteRoute(base, routes);
    handled_.fetch_add(1); // ServeRequest never returns early: always handled
}


void RequestIO::ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes) const {
    string send_target = vermell::http::Response{}
                             .status(404)
                             .type("application/json")
                             .body(R"lit({"error":"this route is not defined"})lit")
                             .str();

    bool head_only = false;

    try {
        const string socket_response = instance->getResponse();

        if (const auto message = vermell::http::Message::parse(socket_response)) {

            head_only = (message->method == "HEAD");

            if (const auto itr = routes->find(route_key(message->path, message->method)); itr != routes->end()) {

                bool guarded;
                {
                    std::lock_guard<std::mutex> lock(itr->second->route_mutex);
                    guarded = TimeGuard(itr);
                    if (guarded)
                        send_target = itr->second->guardRouteMsg != nullptr
                                          ? utility_t::guard_route(itr->second->time_key, *itr->second->guardRouteMsg)
                                          : utility_t::guard_route(itr->second->time_key);
                }

                if (!guarded) {
                    std::unique_ptr<string> guard_msg;
                    auto [data, time_key] = itr->second->middlewares.execute(*message, guard_msg, config_.load()->render);

                    if (time_key > VER_OK) {
                        std::lock_guard<std::mutex> lock(itr->second->route_mutex);
                        itr->second->time_key = time_key;
                        itr->second->time_point = std::chrono::system_clock::now();
                        itr->second->guardRouteMsg = std::move(guard_msg);
                    }
                    send_target = std::move(data);
                }
            }
        } else {
            // The bytes were readable but are not an HTTP request: that is
            // a 400, never a 404 (the route table is not the problem).
            send_target = vermell::http::Response{}
                              .status(400)
                              .type("application/json")
                              .body(R"lit({"error":"malformed request"})lit")
                              .str();
        }
    } catch (const std::exception &e) {
        terminal("REQUEST PROCESSING ERROR: ", e.what());
    }

    // HEAD returns the exact headers a GET would produce (Content-Length
    // included) but never a body (RFC 9110 §9.3.2).
    if (head_only) {
        const size_t sep = send_target.find("\r\n\r\n");
        if (sep != string::npos)
            send_target.resize(sep + 4);
    }

    instance->sendResponse(send_target);

    // The worker owns the fd: this is the single close point of the connection.
    if (close(instance->getDescription()) < enums::neo::eReturn::OK && errno != EBADF)
        terminal(VER_SOCKET_CLOSE, strerror(errno));
}


bool RequestIO::TimeGuard(const RoutesMap::const_iterator &itr) {
    if (itr->second->time_key <= 0)
        return false;
    if (itr->second->time_point == std::chrono::time_point<std::chrono::system_clock>())
        return false;

    const auto now = std::chrono::system_clock::now();

    const std::chrono::duration<double> distance = now - itr->second->time_point;
    return distance.count() < itr->second->time_key;
}


void RequestIO::SetThreads(size_t size) {
    thread_pool_ = make_shared<threading::ThreadPool>(size, config_.load()->max_queue_size);
}


void RequestIO::ApplyConfig(const vermell::Config& config) {
    const auto current = config_.load();
    if (current && config.threads != current->threads) {
        if (config.threads == 0) {
            const unsigned int cores = std::thread::hardware_concurrency();
            SetThreads(cores == 0 ? 8 : cores);
        } else {
            SetThreads(config.threads);
        }
    }
    config_.store(std::make_shared<const vermell::Config>(config));
}
