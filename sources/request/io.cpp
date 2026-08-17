#include "../../include/vermell/request/io.h"

#include <poll.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>

namespace {

    // Maps the parser's framing verdict to a transport status. Message::inspect
    // is the single source of truth for request completeness and validity.
    RequestIO::ReadStatus to_read_status(const vermell::http::Message::Framing framing) noexcept {
        using Framing = vermell::http::Message::Framing;
        switch (framing) {
            case Framing::Complete:        return RequestIO::ReadStatus::Ok;
            case Framing::BadRequest:      return RequestIO::ReadStatus::BadRequest;
            case Framing::TooManyHeaders:  return RequestIO::ReadStatus::TooManyHeaders;
            case Framing::NotImplemented:  return RequestIO::ReadStatus::NotImplemented;
            default:                       return RequestIO::ReadStatus::Failed; // Incomplete
        }
    }

    // Decrements the connection counter when a worker is done with a fd, on
    // every exit path of HandleClient (including early returns).
    struct ConnectionGuard {
        std::atomic<size_t>& counter;
        explicit ConnectionGuard(std::atomic<size_t>& c) : counter(c) {}
        ~ConnectionGuard() { counter.fetch_sub(1); }
    };

}

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
                                                   config_(config) {

    thread_pool_ = make_shared<threading::ThreadPool>(threads_, config_.max_queue_size);
}


void RequestIO::Dispatch(const int notice) const {

    if (notice <= 0)
        return;

    for (int i = 0; i < notice; i++) {

        const int event_fd = events->operator[](i).data.fd;
        const uint32_t event_mask = events->operator[](i).events;

        if (event_fd == *file_descriptor) {
            AcceptPending();
            continue;
        }

        if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
            epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);
            close(event_fd);
            active_connections_.fetch_sub(1);
            continue;
        }

        if (!(event_mask & EPOLLIN))
            continue;

        // Remove the fd from epoll BEFORE handing it to the pool: from this
        // point on a single worker owns the fd exclusively, so no other
        // thread can read/close it behind our back.
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);

        try {
            thread_pool_->addTask([this, event_fd]() {
                this->HandleClient(event_fd);
            });
        } catch (const std::exception &e) {
            terminal("THREAD POOL REJECTED TASK: ", e.what());
            close(event_fd);
            active_connections_.fetch_sub(1);
        }
    }
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
        if (config_.max_connections != 0
            && active_connections_.load() >= config_.max_connections) {
            close(client_file_descriptor);
            continue;
        }

        if (Server::setNonblocking(client_file_descriptor) == VER_SOCKET_ERROR) {
            close(client_file_descriptor);
            continue;
        }

        epoll_event client_event{};
        client_event.events = EPOLLIN; // level triggered: it is removed from epoll on dispatch
        client_event.data.fd = client_file_descriptor;

        if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, client_file_descriptor, &client_event) == VER_NVALUE) {
            terminal(VER_EPOLL_CERR, strerror(errno));
            close(client_file_descriptor);
            continue;
        }

        active_connections_.fetch_add(1);
    }
}


void RequestIO::HandleClient(const int event_fd) const {

    // This fd was accepted (counted) and now belongs exclusively to this
    // worker; the guard releases the count on every exit path.
    ConnectionGuard guard(active_connections_);

    string raw_request;

    const auto base = std::make_shared<Server>();

    base->setPort(connection->getPort());
    base->setSocketId(event_fd);
    base->setWriteTimeout(config_.write_timeout);

    const ReadStatus status = ReadRequest(event_fd, raw_request);

    if (status != ReadStatus::Ok || raw_request.empty()) {
        // Tell the client WHY instead of silently dropping the connection.
        int code = 0;
        const char* error = nullptr;
        switch (status) {
            case ReadStatus::TooLarge:       code = 413; error = "payload too large"; break;
            case ReadStatus::BadRequest:     code = 400; error = "malformed request"; break;
            case ReadStatus::TooManyHeaders: code = 431; error = "too many headers"; break;
            case ReadStatus::NotImplemented: code = 501; error = "transfer encoding not supported"; break;
            default: break; // Failed: the socket is broken, nothing can be sent
        }
        if (code != 0)
            base->sendResponse(vermell::http::Response{}
                                   .status(code)
                                   .type("application/json")
                                   .body(std::string(R"lit({"error":")lit") + error + R"lit("})lit")
                                   .str());
        close(event_fd);
        return;
    }

    base->setResponse(raw_request);

    ExecuteRoute(base, routes);
}


RequestIO::ReadStatus RequestIO::ReadRequest(const int event_fd, string &out) const {

    vector<char> chunk(config_.read_chunk);

    for (;;) {
        const ssize_t bytes = recv(event_fd, chunk.data(), chunk.size(), 0);

        if (bytes > 0) {
            out.append(chunk.data(), static_cast<size_t>(bytes));

            if (out.size() > config_.max_request_size)
                return ReadStatus::TooLarge;

            const auto inspection = vermell::http::Message::inspect(out);
            if (inspection.framing == vermell::http::Message::Framing::Incomplete) {
                // The client promised a body bigger than the whole-request
                // cap: reject right after the head instead of reading it all.
                if (inspection.expected > config_.max_request_size)
                    return ReadStatus::TooLarge;
                continue;
            }
            return to_read_status(inspection.framing);
        }

        if (bytes == VER_OK)
            break; // peer closed: the request must stand on its own

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            pollfd pfd{};
            pfd.fd = event_fd;
            pfd.events = POLLIN;

            // poll() takes an int: clamp so a misconfigured timeout can
            // neither wait forever (slow-client DoS) nor overflow.
            const auto timeout_ms = std::clamp(config_.read_timeout.count(),
                                               std::chrono::milliseconds::rep{1},
                                               static_cast<std::chrono::milliseconds::rep>(std::numeric_limits<int>::max()));
            const int ready = poll(&pfd, 1, static_cast<int>(timeout_ms));

            if (ready > 0 && (pfd.revents & (POLLIN | POLLHUP)))
                continue;

            break; // inactivity timeout: the request must stand on its own
        }

        return ReadStatus::Failed;
    }

    // The connection gave us everything it ever will. A request that is
    // still Incomplete (e.g. a Content-Length body that never arrived) is
    // not "whatever arrived": it is a malformed request.
    if (out.empty())
        return ReadStatus::Failed;

    const auto inspection = vermell::http::Message::inspect(out);
    return inspection.framing == vermell::http::Message::Framing::Incomplete
               ? ReadStatus::BadRequest
               : to_read_status(inspection.framing);
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
                    auto [data, time_key] = itr->second->middlewares.execute(*message, guard_msg, config_.render);

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
    thread_pool_ = make_shared<threading::ThreadPool>(size, config_.max_queue_size);
}
