#include "../../include/vibe/request/io.h"

RequestIO::RequestIO(const std::shared_ptr<RoutesMap> &routes,
                     int &filed,
                     int &epoll_fd, const neo::LISTEN_TYPE _type,
                     const std::function<void()> &callback
                     ) : file_descriptor(std::make_unique<int>(filed)),
                                                                                                              epoll_fd(std::make_unique<int>(epoll_fd)),
                                                                                                              routes(routes), listen_type(_type){

    thread_pool_ = make_shared<threading::ThreadPool>(threads_, 200);
    fd_validate = make_shared<FdValidate>(epoll_fd);
    taskManager = make_unique<TaskManager>(fd_validate);
    parent_callback = callback;

    step_process = []()->void {};

    if(_type == neo::UNIQUE) {

        step_process = [&]() -> void {

            general_increment.store(general_increment.load() + 1);

            if(general_increment.load() == STEP_TO_KILL) {
                if(parent_callback) {
                    parent_callback();
                }
                taskManager->kill();
                thread_pool_->kill();
            }
        };
    }

}



void RequestIO::Dispatch(const int _list, const shared_ptr<std::vector<epoll_event>>& events)  {

    const string time_key = std::to_string(neosys::process::random());

    taskManager->manage(time_key,  thread_pool_->addFutureTask([this, _list, events, time_key](const shared_ptr<std::promise<void>>& future)->void {

                      this->Process(_list,  time_key, events);
        future->set_value();

    }));

}


void RequestIO::Process(const int list, const string& key, const shared_ptr<std::vector<epoll_event>>& events) {
    constexpr auto socket_len_error_value = static_cast<socklen_t>(-1);

    for (int i = 0; i < list; i++) {
        const int event_fd = events->operator[](i).data.fd;

        if (event_fd == *file_descriptor) {

            while (true) {
                sockaddr_in client_addr{};
                socklen_t client_address_len = sizeof(client_addr);
                int client_fd = accept(*file_descriptor,
                                       reinterpret_cast<sockaddr*>(&client_addr),
                                       &client_address_len);

                if (client_fd == VB_NVALUE) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    terminal(VB_EPOLL_CERR, strerror(errno));
                    break;
                }

                if (client_address_len == socket_len_error_value) {
                    ::close(client_fd);
                    continue;
                }

                if (Server::setNonblocking(client_fd) != MG_OK) {
                    ::close(client_fd);
                    continue;
                }

                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                ev.data.fd = client_fd;
                fd_validate->add(client_fd, ev);
            }

        } else {

            if (!FdValidate::is_valid(event_fd) || fd_validate->isBusy(event_fd)) {
                continue;
            }

            fd_validate->busy(event_fd);

           const auto state =  ProcessFileDescriptor(event_fd, key);
            fd_validate->unBusy(event_fd);
            // fd_validate->dispose(event_fd);
            if (state)
                fd_validate->rearm(event_fd);
            else
                fd_validate->disposeBase(event_fd);

            // fd_validate->dispose(event_fd);

            // epoll_event ev{};
            // ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
            // ev.data.fd = event_fd;
            // if (epoll_ctl(*file_descriptor, EPOLL_CTL_MOD, event_fd, &ev) == -1) {
            //     terminal(VB_EPOLL_CERR, strerror(errno));
            //     fd_validate->dispose(event_fd);
            // }
        }
    }
}




bool RequestIO::ProcessFileDescriptor(const int event_fd, const string& key) {

    std::lock_guard<std::mutex> guard{mutex_fd};
    if (!FdValidate::is_valid(event_fd)) {
        return false;
    }

    std::vector<char> recvBuffer;
    std::array<char, DEF_BUFFER_SIZE> buffer{};

    while (true) {
        const ssize_t bytes = recv(event_fd, buffer.data(), buffer.size(), VB_OK);
        if (bytes == VB_NVALUE) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            terminal(VB_EPOLL_CERR, strerror(errno));
            // fd_validate->dispose(event_fd);
            return false;
        } else if (bytes == 0) {
            // fd_validate->dispose(event_fd);
            return false;
        }
        recvBuffer.insert(recvBuffer.end(), buffer.begin(), buffer.begin() + bytes);
    }
    if (recvBuffer.empty()) {
        return false;
    }

    std::array<char, DEF_BUFFER_SIZE> requestArray{};
    const size_t len = std::min(recvBuffer.size(), requestArray.size());
    std::copy_n(recvBuffer.data(), len, requestArray.data());

    return ExecuteRoute(event_fd, key, requestArray, routes);
}






bool RequestIO::ExecuteRoute(const int client_fd, const string& key, const std::array<char, DEF_BUFFER_SIZE> &buffer, const shared_ptr<RoutesMap> &routes) const
{

    string send_target = HttpUtils::create_response("<h1>the resource could not be accessed</h1>", "text/html");

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

    if (const auto send_= Server::sendResponse(send_target,*epoll_fd ,client_fd); !send_)
        terminal(VB_EPOLL_CERR, VB_SOCKET_SEND);
    else
        taskManager->releaseOne(key);

    step_process();

    return true;
}



bool RequestIO::TimeGuard(const unique_ptr<listen_routes> &itr) {

    const auto now = system_clock::now();
    const auto distance = std::chrono::duration_cast<milliseconds>(now - itr->time_point);

    return distance < itr->time_key;
}

void RequestIO::SetThreads(size_t size) {
    thread_pool_ = make_shared<threading::ThreadPool>(size, 128);
}


