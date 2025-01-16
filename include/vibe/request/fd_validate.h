#ifndef FD_VALIDATE_H
#define FD_VALIDATE_H

#include <unistd.h>
#include <mutex>
#include <unordered_map>
#include <sys/epoll.h>
#include <fcntl.h>

class FdValidate{

    std::mutex access_;
    std::unordered_map<int, bool> inuse_;

  public:

    FdValidate();
    ~FdValidate();

    void dispose(int fd);
    void unsafeDispose(int fd) noexcept;

    [[nodiscard]] static bool is_valid(int) noexcept;

    void busy(int fd) noexcept;
    void unBusy(int fd) noexcept;
    bool isBusy(int fd) noexcept;
    void free(int fd) noexcept;

};

#endif
