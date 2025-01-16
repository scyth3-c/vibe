#include "../../include/vibe/request/fd_validate.h"

FdValidate::FdValidate()
{
    inuse_ = std::unordered_map<int,bool>();
}
FdValidate::~FdValidate()
{
    inuse_.clear();
}


bool FdValidate::is_valid(const int fd) noexcept {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

void FdValidate::dispose(const int fd)
{
    std::lock_guard<std::mutex> lock(access_);
    if (inuse_.find(fd) == inuse_.end()) {
        if (is_valid(fd)) {
            epoll_ctl(fd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
        }
    }
}


void FdValidate::unsafeDispose(const int fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    if (is_valid(fd)) {
        epoll_ctl(fd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }
}

void FdValidate::busy(const int fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    inuse_[fd] = true;
}

void FdValidate::unBusy(const int fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    inuse_[fd] = false;
}

bool FdValidate::isBusy(const int fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    return inuse_.find(fd) != inuse_.end();
}

void FdValidate::free(const int fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    inuse_.erase(fd);
}
