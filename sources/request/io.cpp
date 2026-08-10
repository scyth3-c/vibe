#include "../../include/vibe/request/io.h"

#include <poll.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace {

    // Best-effort check for a fully received HTTP request: complete header
    // block and, when present, the whole Content-Length body.
    bool request_complete(const string &req) {

        size_t header_end = req.find("\r\n\r\n");
        size_t separator = 4;

        if (header_end == string::npos) {
            header_end = req.find("\n\n");
            separator = 2;
        }

        if (header_end == string::npos)
            return false;

        string headers = req.substr(0, header_end);
        std::transform(headers.begin(), headers.end(), headers.begin(),
                       [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

        constexpr auto content_length_key = "content-length:";
        constexpr size_t key_length = 15; // strlen("content-length:")

        const size_t key_pos = headers.find(content_length_key);
        if (key_pos == string::npos)
            return true;

        const size_t value_begin = headers.find_first_not_of(" \t", key_pos + key_length);
        if (value_begin == string::npos)
            return true;

        const long body_length = std::strtol(headers.c_str() + value_begin, nullptr, 10);
        if (body_length <= 0)
            return true;

        return req.size() >= header_end + separator + static_cast<size_t>(body_length);
    }
}

RequestIO::RequestIO(const shared_ptr<vector<epoll_event> > &events,
                     const std::shared_ptr<RoutesMap> &routes,
                     int &filed,
                     int &epoll_fd,
                     const shared_ptr<Server> &con,
                     const vibe::Config &config) : events(events),
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

        if (client_file_descriptor == VB_NVALUE) {
            if (errno == EINTR)
                continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                terminal(VB_EPOLL_CERR, strerror(errno));
            return;
        }

        if (Server::setNonblocking(client_file_descriptor) == MG_ERROR) {
            close(client_file_descriptor);
            continue;
        }

        epoll_event client_event{};
        client_event.events = EPOLLIN; // level triggered: it is removed from epoll on dispatch
        client_event.data.fd = client_file_descriptor;

        if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, client_file_descriptor, &client_event) == VB_NVALUE) {
            terminal(VB_EPOLL_CERR, strerror(errno));
            close(client_file_descriptor);
        }
    }
}


void RequestIO::HandleClient(const int event_fd) const {

    string raw_request;

    const auto base = std::make_shared<Server>();

    base->setPort(connection->getPort());
    base->setSocketId(event_fd);
    base->setWriteTimeout(config_.write_timeout);

    const ReadStatus status = ReadRequest(event_fd, raw_request);

    if (status == ReadStatus::TooLarge) {
        // Inform the client instead of silently dropping the connection.
        base->sendResponse(vibe::http::Response{}
                               .status(413)
                               .type("application/json")
                               .body(R"lit({"error":"payload too large"})lit")
                               .str());
        close(event_fd);
        return;
    }

    if (status != ReadStatus::Ok || raw_request.empty()) {
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

            if (request_complete(out))
                return ReadStatus::Ok;

            continue;
        }

        if (bytes == VB_OK)
            return out.empty() ? ReadStatus::Failed : ReadStatus::Ok; // peer closed: use whatever arrived

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            pollfd pfd{};
            pfd.fd = event_fd;
            pfd.events = POLLIN;

            const int ready = poll(&pfd, 1, static_cast<int>(config_.read_timeout.count()));

            if (ready > 0 && (pfd.revents & (POLLIN | POLLHUP)))
                continue;

            return out.empty() ? ReadStatus::Failed : ReadStatus::Ok; // timeout: use whatever arrived
        }

        return ReadStatus::Failed;
    }
}


void RequestIO::ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes) const {
    string send_target = vibe::http::Response{}
                             .status(404)
                             .type("application/json")
                             .body(R"lit({"error":"this route is not defined"})lit")
                             .str();

    try {
        const string socket_response = instance->getResponse();

        if (const auto message = vibe::http::Message::parse(socket_response)) {

            if (const auto itr = routes->find(message->path + message->method); itr != routes->end()) {

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

                    if (time_key > VB_OK) {
                        std::lock_guard<std::mutex> lock(itr->second->route_mutex);
                        itr->second->time_key = time_key;
                        itr->second->time_point = std::chrono::system_clock::now();
                        itr->second->guardRouteMsg = std::move(guard_msg);
                    }
                    send_target = std::move(data);
                }
            }
        }
    } catch (const std::exception &e) {
        terminal("REQUEST PROCESSING ERROR: ", e.what());
    }

    instance->sendResponse(send_target);

    // The worker owns the fd: this is the single close point of the connection.
    if (close(instance->getDescription()) < enums::neo::eReturn::OK && errno != EBADF)
        terminal(VB_SOCKET_CLOSE, strerror(errno));
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
