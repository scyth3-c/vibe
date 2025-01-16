//
// Created by scythe on 5/07/23.
//

#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include <stdexcept>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>

#include "../util/enums.h"
#include "../util/parameter_proccess.h"

#include "../routes.hpp"
#include "../util/nterminal.h"
#include "io.h"

namespace workers {
using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;
using enums::neo;

    constexpr int BUFFER = neo::eSize::BUFFER;
    constexpr int SESSION = neo::eSize::SESSION;
    constexpr int INIT_MAX_EVENTS = 10;

    template<class T>
    class RouterEpoll {

        int epoll_fd, file_descriptor;
        shared_ptr<std::vector<epoll_event>> events;
        shared_ptr<T> connection;
        shared_ptr<RequestIO> request_t;
        neo::eStatus listen_status_;

    public:

        explicit RouterEpoll(const shared_ptr<T> &conn) :
        epoll_fd(epoll_create1(0)), file_descriptor(-1),
        events(make_shared<vector<epoll_event>>(INIT_MAX_EVENTS)),
        connection(conn),
        listen_status_(enums::neo::eStatus::START)
        {}


        auto InitListenProcess() {
            connection->on();
            file_descriptor = connection->getDescription();

            if(Server::setNonblocking(file_descriptor) == MG_ERROR)
                close(file_descriptor);

            if (epoll_fd == -1)
                throw std::range_error(VB_EPOLL_RANGE);
        }


        auto ListenProcess(epoll_event &event) const {
            try {
                const int notice = epoll_wait(epoll_fd, events->data(), INIT_MAX_EVENTS, VB_NVALUE);
                request_t->Dispatch(notice, event);
            }
            catch(const std::exception& e) {
                terminal(e.what());
                close(epoll_fd);
                if(close(file_descriptor) < VB_OK)
                    throw std::range_error(VB_MAIN_THREAD);
            }
        }

        auto getMainProcess(const shared_ptr<RoutesMap> &_routes, const neo::LISTEN_TYPE _listen_type = neo::WHILE) {
                InitListenProcess();

                epoll_event event{};
                event.events = EPOLLIN;
                event.data.fd = file_descriptor;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) == VB_NVALUE) {
                    close(epoll_fd);
                    throw std::range_error(VB_EPOLL_CTL);
                }
                request_t = make_shared<RequestIO>(events, _routes, file_descriptor, epoll_fd, connection);

                if (_listen_type == neo::WHILE) {
                    while (static_cast<bool>(listen_status_)) {
                        ListenProcess(event);
                    }
                    close(epoll_fd);
                    close(file_descriptor);
                    request_t.reset();
                }
                else {
                        // UNIQUE
                        ListenProcess(event);
                        ListenProcess(event);
                        close(epoll_fd);
                        close(file_descriptor);
                        request_t.reset();
                }
        }

        void setListenStatus(const neo::eStatus _status) {
            this->listen_status_ = _status;
        }
    };
}


#endif //MAIN_PROCESS_H
