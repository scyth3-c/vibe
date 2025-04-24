#ifndef FD_VALIDATE_H
#define FD_VALIDATE_H

#include <unistd.h>
#include <mutex>
#include <unordered_map>
#include <sys/epoll.h>
#include <chrono>
#include <fcntl.h>
#include "../sysop/literals.h"
#include "../abstract.hpp"
#include "../configuration.hpp"

using namespace std::chrono;

class FdValidate{

    std::mutex access_;
    std::unordered_map<int, FDInfo > inuse_;
    int epoll_fd;

  public:

    explicit FdValidate(int);
    ~FdValidate();


    void dispose(int client_fd);
    void disposeBase(int client_fd) const;

    [[nodiscard]] static bool is_valid(int) noexcept;

    void busy(int client_fd) noexcept;
    void unBusy(int client_fd) noexcept;
    bool isBusy(int client_fd) noexcept;

    void clearOldFd();


    void add(int client_fd, epoll_event&)  noexcept;

};

#endif
