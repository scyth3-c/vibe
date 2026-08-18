#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include <stdexcept>
#include <memory>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <limits>
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
// RoutesMap lives in routes.hpp (transparent hashing for allocation-free
// lookups); it is visible here through the include chain.
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
            if (connection->on() != VER_SOCKET_OK)
                throw std::runtime_error("AN ERROR OCCURRED WHEN INITIALIZING THE SERVER SOCKET");

            file_descriptor = connection->getDescription();

            if(file_descriptor < 0) {
                connection->Close();
                throw std::runtime_error(VER_MAIN_THREAD);
            }

            if(Server::setNonblocking(file_descriptor) == VER_SOCKET_ERROR) {
                connection->Close();
                file_descriptor = -1;
                throw std::runtime_error(VER_MAIN_THREAD);
            }

            if (epoll_fd == -1) {
                connection->Close();
                file_descriptor = -1;
                throw std::range_error(VER_EPOLL_RANGE);
            }
        }


        auto ListenProcess(const int timeout) const {
            const int notice = epoll_wait(epoll_fd, events->data(), static_cast<int>(events->size()), timeout);

            if (notice == VER_NVALUE) {
                if (errno == EINTR)
                    return;
                throw std::runtime_error(std::string(VER_EPOLL_CERR) + strerror(errno));
            }

            request_t->Dispatch(notice);
        }

        auto getMainProcess(const shared_ptr<RoutesMap> &_routes,
                            const neo::LISTEN_TYPE _listen_type = neo::WHILE,
                            const vermell::Config &config = {}) {
                InitListenProcess();

                // Apply the configured epoll batch size before waiting.
                if (config.max_events > 0
                    && static_cast<size_t>(config.max_events) != events->size())
                    events = make_shared<vector<epoll_event>>(static_cast<size_t>(config.max_events));

                epoll_event event{};
                event.events = EPOLLIN;
                event.data.fd = file_descriptor;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) == VER_NVALUE) {
                    close(epoll_fd);
                    epoll_fd = -1;
                    connection->Close();
                    file_descriptor = -1;
                    throw std::range_error(VER_EPOLL_CTL);
                }
                request_t = make_shared<RequestIO>(events, _routes, file_descriptor, epoll_fd, connection, config);

                // epoll_wait takes an int timeout: clamp so a misconfigured
                // value cannot overflow the conversion (negative would mean
                // "wait forever" and stall the stop/status loop).
                const auto epoll_timeout_ms = std::clamp(config.epoll_timeout.count(),
                                                         std::chrono::milliseconds::rep{1},
                                                         static_cast<std::chrono::milliseconds::rep>(std::numeric_limits<int>::max()));
                const auto wait_timeout = static_cast<int>(epoll_timeout_ms);

                try {
                    if (_listen_type == neo::WHILE) {
                        while (listen_status_.load() == neo::eStatus::START) {
                            ListenProcess(wait_timeout);
                        }
                    }
                    else {
                        // UNIQUE: serve one request, then stop. Loop on a
                        // BOUNDED wait until a connection was fully handled:
                        // a fast client whose accept and data events land in
                        // the same epoll batch must not leave a second
                        // infinite epoll_wait blocked forever.
                        do {
                            ListenProcess(wait_timeout);
                        } while (request_t->handled_connections() == 0);
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

        // Applies a new configuration to a running server (timeouts, limits,
        // thread count). No-op before the first listen().
        void applyConfig(const vermell::Config& config) {
            if (request_t != nullptr)
                request_t->ApplyConfig(config);
        }
    };
}


#endif //MAIN_PROCESS_H
