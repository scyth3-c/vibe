//
// Created by scythe on 5/07/23.
//

#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include <stdexcept>
#include <memory>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>

#include "../util/enums.h"
#include "../util/parameter_proccess.h"

#include "../config.hpp"
#include "../routes.hpp"
#include "../util/nterminal.h"
#include "io.h"

namespace workers {
using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;
using enums::neo;

    constexpr int BUFFER = neo::eSize::BUFFER;
    constexpr int SESSION = neo::eSize::SESSION;
    constexpr int INIT_MAX_EVENTS = 1024;
    // Wake up periodically so setListenStatus(STOP) is actually honored.
    constexpr int EPOLL_TIMEOUT_MS = 1000;

    template<class T>
    class RouterEpoll {

        int epoll_fd, file_descriptor;
        shared_ptr<std::vector<epoll_event>> events;
        shared_ptr<T> connection;
        shared_ptr<RequestIO> request_t;
        std::atomic<neo::eStatus> listen_status_;

    public:

        explicit RouterEpoll(const shared_ptr<T> &conn) :
        epoll_fd(epoll_create1(0)), file_descriptor(-1),
        events(make_shared<vector<epoll_event>>(INIT_MAX_EVENTS)),
        connection(conn),
        listen_status_(enums::neo::eStatus::START)
        {}

        ~RouterEpoll() {
            // epoll_fd is owned here: getMainProcess() resets it to -1 after
            // closing, so a router that never listened does not leak the fd.
            if (epoll_fd >= 0)
                close(epoll_fd);
        }


        auto InitListenProcess() {
            if (connection->on() != MG_OK)
                throw std::runtime_error("AN ERROR OCCURRED WHEN INITIALIZING THE SERVER SOCKET");

            file_descriptor = connection->getDescription();

            if(file_descriptor < 0) {
                connection->Close();
                throw std::runtime_error(VB_MAIN_THREAD);
            }

            if(Server::setNonblocking(file_descriptor) == MG_ERROR) {
                connection->Close();
                file_descriptor = -1;
                throw std::runtime_error(VB_MAIN_THREAD);
            }

            if (epoll_fd == -1) {
                connection->Close();
                file_descriptor = -1;
                throw std::range_error(VB_EPOLL_RANGE);
            }
        }


        auto ListenProcess(const int timeout) const {
            const int notice = epoll_wait(epoll_fd, events->data(), static_cast<int>(events->size()), timeout);

            if (notice == VB_NVALUE) {
                if (errno == EINTR)
                    return;
                throw std::runtime_error(std::string(VB_EPOLL_CERR) + strerror(errno));
            }

            request_t->Dispatch(notice);
        }

        auto getMainProcess(const shared_ptr<RoutesMap> &_routes,
                            const neo::LISTEN_TYPE _listen_type = neo::WHILE,
                            const vibe::Config &config = {}) {
                InitListenProcess();

                // Apply the configured epoll batch size before waiting.
                if (config.max_events > 0
                    && static_cast<size_t>(config.max_events) != events->size())
                    events = make_shared<vector<epoll_event>>(static_cast<size_t>(config.max_events));

                epoll_event event{};
                event.events = EPOLLIN;
                event.data.fd = file_descriptor;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) == VB_NVALUE) {
                    close(epoll_fd);
                    epoll_fd = -1;
                    connection->Close();
                    file_descriptor = -1;
                    throw std::range_error(VB_EPOLL_CTL);
                }
                request_t = make_shared<RequestIO>(events, _routes, file_descriptor, epoll_fd, connection, config);

                const auto wait_timeout = static_cast<int>(config.epoll_timeout.count());

                try {
                    if (_listen_type == neo::WHILE) {
                        while (listen_status_.load() == neo::eStatus::START) {
                            ListenProcess(wait_timeout);
                        }
                    }
                    else {
                        // UNIQUE: block until the connection and its request arrive
                        ListenProcess(VB_NVALUE);
                        ListenProcess(VB_NVALUE);
                    }
                }
                catch(const std::exception& e) {
                    terminal(e.what());
                }

                // request_t joins the pool, so in-flight responses finish first.
                request_t.reset();
                close(epoll_fd);
                epoll_fd = -1;
                // Close through the owner so its descriptor state is cleared:
                // a raw close() here would leave the Server holding a stale fd.
                connection->Close();
                file_descriptor = -1;
        }

        void setListenStatus(const neo::eStatus _status) {
            this->listen_status_.store(_status);
        }
    };
}


#endif //MAIN_PROCESS_H
