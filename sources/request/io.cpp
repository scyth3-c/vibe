#include "../../include/vibe/request/io.h"

RequestIO::RequestIO(const shared_ptr<vector<epoll_event> > &events,
                     const std::shared_ptr<RoutesMap> &routes,
                     int &filed,
                     int &epoll_fd,
                     const shared_ptr<Server> &con) : file_descriptor(std::make_unique<int>(filed)),
                                                      epoll_fd(std::make_unique<int>(epoll_fd)),
                                                      events(events),
                                                      routes(routes),
                                                      connection(con) {

    thread_pool_ = make_shared<threading::ThreadPool>(threads_);
    taskManager = make_unique<TaskManager>();
}


void RequestIO::Dispatch(const int id, epoll_event event) const {

    std::shared_future<void> future = thread_pool_->addTask([this, id, event]()->void {
                this->Process(id, event);
                taskManager->notifyOne(id);
    });

    future.get();
    // taskManager->addTask(0, future);
}


void RequestIO::Process(const int id, epoll_event event) const {

    if (id == -1) {
        terminal(VB_EPOLL_CERR, strerror(errno));
        return;
    }

    constexpr auto socket_len_error_value = static_cast<socklen_t>(-1);

    for (int i = 0; i < id; i++) {

        if (const int event_fd = events->operator[](i).data.fd;
            event_fd == *file_descriptor
        ) {
            sockaddr_in client_addr{};
            socklen_t client_adrr_len = sizeof(client_addr);

            int client_file_descriptor = accept(*file_descriptor, reinterpret_cast<sockaddr *>(&client_addr),
                                                &client_adrr_len);

            if (client_adrr_len == socket_len_error_value)
                continue;

            Server::setNonblocking(client_file_descriptor);

            event.events = EPOLLIN | EPOLLET;
            event.data.fd = client_file_descriptor;

            if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, client_file_descriptor, &event) == VB_NVALUE) {
                terminal(VB_EPOLL_CERR, strerror(errno));
                close(client_file_descriptor);
            }

        } else
                ProcessFileDescriptor(event_fd, id);
    }
}




void RequestIO::ProcessFileDescriptor(const int event_fd, const int notice) const {

    std::array<char, DEF_BUFFER_SIZE> buffer{};

    if (const ssize_t bytes = recv(event_fd, buffer.data(), buffer.size(), VB_OK); bytes == VB_NVALUE) {

        if (errno == EWOULDBLOCK)
            return;

        terminal(VB_EPOLL_CERR, strerror(errno));
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);
        close(event_fd);

    } else if (bytes == VB_OK) {

        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);
        close(event_fd);

    } else {
        const auto base = std::make_shared<Server>();

        base->setPort(connection->getPort());
        base->setSocketId(event_fd);
        base->setResponse(buffer);
        base->setEpollEvents(*events);
        base->setEpollfd(*epoll_fd);
        base->setNotices(notice);

        ExecuteRoute(base, routes);
    }
}





void RequestIO::ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes) {
    string send_target = ERROR_GET;

    const string socket_response = instance->getResponse();
    if (socket_response.empty()) throw std::range_error(VB_SOCKET_FAIL);


    auto [type, route] = HTTP_QUERY::route_refactor(socket_response);

    if (const auto itr = routes->find(route + type); itr != routes->end()) {
        if (not TimeGuard(itr)) {
            const string parameters = type == GET_TYPE
                                          ? HTTP_QUERY::route_refactor_params_get(socket_response)
                                          : HTTP_QUERY::route_refactor_params(socket_response);

            auto [data, time_key] = itr->second->middlewares.execute(parameters,
                                                                     HTTP_QUERY::headers_from(socket_response),
                                                                     itr->second->guardRouteMsg
            );

            if (time_key > VB_OK) {
                itr->second->time_key = time_key;
                itr->second->time_point = std::chrono::system_clock::now();
            }
            send_target = std::move(data);
        } else
            send_target = itr->second->guardRouteMsg != nullptr
                              ? utility_t::guard_route(itr->second->time_key, *itr->second->guardRouteMsg)
                              : utility_t::guard_route(itr->second->time_key);
    }


    instance->sendResponse(send_target);

    if (close(instance->getDescription()) < enums::neo::eReturn::OK) {
        throw std::range_error(VB_SOCKET_CLOSE);
    }
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
    thread_pool_ = make_shared<threading::ThreadPool>(size);
}
