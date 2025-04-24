#include "../../include/vibe/request/io.h"

RequestIO::RequestIO(const std::shared_ptr<RoutesMap> &routes,
                     int &filed,
                     int &epoll_fd,
                     const shared_ptr<Server> &con) : file_descriptor(std::make_unique<int>(filed)),
                                                      epoll_fd(std::make_unique<int>(epoll_fd)),
                                                      routes(routes),
                                                      connection(con) {

    thread_pool_ = make_shared<threading::ThreadPool>(threads_);
    fd_validate = make_shared<FdValidate>(epoll_fd);
    taskManager = make_unique<TaskManager>(fd_validate);

}



void RequestIO::Dispatch(const int _list, const shared_ptr<std::vector<epoll_event>>& events)  {

     const string key = std::to_string(process::random());

    taskManager->manage(key,  thread_pool_->addFutureTask([this, _list, events, key](const shared_ptr<std::promise<void>>& future)->void {

                      this->Process(_list, events);

        future->set_value();
        taskManager->releaseOne(key);

     }));

}


void RequestIO::Process(const int list, const shared_ptr<std::vector<epoll_event>>& events)  {

    constexpr auto socket_len_error_value = static_cast<socklen_t>(-1);

    for (int i = 0; i < list ; i++) {

        if (const int event_fd = events->operator[](i).data.fd;
            event_fd == *file_descriptor
        ) {

            sockaddr_in client_addr{};
            socklen_t client_address_len = sizeof(client_addr);

            int client_file_descriptor = accept(*file_descriptor, reinterpret_cast<sockaddr *>(&client_addr),
                                                &client_address_len);

            if (client_address_len == socket_len_error_value)
                continue;

            if (client_file_descriptor  == VB_NVALUE) {
              if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EBADF)
                  continue;
              terminal(VB_EPOLL_CERR, errno);
            }

            if (Server::setNonblocking(client_file_descriptor) != MG_OK) {
                close(client_file_descriptor);
                continue;
            }

            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = client_file_descriptor;

            fd_validate->add(client_file_descriptor, ev);

        } else {

            if (!FdValidate::is_valid(event_fd) || fd_validate->isBusy(event_fd)) {
                continue;
            }

            fd_validate->busy(event_fd);

            ProcessFileDescriptor(event_fd);

            fd_validate->unBusy(event_fd);
            fd_validate->dispose(event_fd);

        }

    }
}




void RequestIO::ProcessFileDescriptor(const int event_fd)  {


    std::lock_guard<std::mutex> guard{mutex_fd};
    if (!FdValidate::is_valid(event_fd)) {
        return;
    }

    std::array<char, DEF_BUFFER_SIZE> buffer{};

    if (const ssize_t bytes = recv(event_fd, buffer.data(), buffer.size(), VB_OK); bytes == VB_NVALUE)
    {
        if (errno == EWOULDBLOCK)
            return;

        terminal(VB_EPOLL_CERR, strerror(errno));

        fd_validate->disposeBase(event_fd);


    } else if (bytes == VB_OK) {

        fd_validate->disposeBase(event_fd);

    } else {

        ExecuteRoute(event_fd, buffer, routes);
    }
}





void RequestIO::ExecuteRoute(const int client_fd, std::array<char, DEF_BUFFER_SIZE> buffer, const shared_ptr<RoutesMap> &routes) const
{

    string send_target = HttpUtils::create_response("<h1>error</h1>", "text/html");

    const string socket_response(buffer.begin(), buffer.size());

    if (socket_response.empty()) throw std::range_error(VB_SOCKET_FAIL);


    auto [type, route] = HTTP_QUERY::route_refactor(socket_response);

    if (const auto itr = routes->find(route + type); itr != routes->end()) {
        if (not TimeGuard(itr->second)) {
            const string parameters = type == GET_TYPE
                                          ? HTTP_QUERY::route_refactor_params_get(socket_response)
                                          : HTTP_QUERY::route_refactor_params(socket_response);

            auto [data, time_key] = itr->second->middlewares.execute(parameters,
                                                                     HTTP_QUERY::headers_from(socket_response),
                                                                     itr->second->guardRouteMsg
            );

            if (time_key > milliseconds(0)) {
                itr->second->time_key = milliseconds(time_key);
                itr->second->time_point = high_resolution_clock::now();
            }
            send_target = std::move(data);
        } else
            send_target = itr->second->guardRouteMsg != nullptr
                              ? HttpUtils::rate_limit_response(itr->second->time_key, *itr->second->guardRouteMsg)
                              : HttpUtils::rate_limit_response(itr->second->time_key);
    }

    if (const auto send_= connection->sendResponse(send_target, client_fd); !send_)
        terminal(VB_EPOLL_CERR, VB_SOCKET_SEND);
}



bool RequestIO::TimeGuard(const unique_ptr<listen_routes> &itr) {

    const auto now = system_clock::now();
    const auto distance = std::chrono::duration_cast<milliseconds>(now - itr->time_point);

    return distance < itr->time_key;
}

void RequestIO::SetThreads(size_t size) {
    thread_pool_ = make_shared<threading::ThreadPool>(size);
}
