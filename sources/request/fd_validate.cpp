#include "../../include/vibe/request/fd_validate.h"

FdValidate::FdValidate(const int _fd)
{
    inuse_ = std::unordered_map<int,FDInfo>();
    epoll_fd = _fd;
}
FdValidate::~FdValidate()
{
    inuse_.clear();
}


bool FdValidate::is_valid(const int fd) noexcept {
    return fcntl(fd, F_GETFD) != -1;
}


void FdValidate::disposeBase(const int client_fd) const {

    if (!is_valid(client_fd) || client_fd == epoll_fd )
        return;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
        terminal(VB_EPOLL_CERR,  "-dispose epoll FD-");
    }

    if (::close(client_fd) == -1) {
        terminal(VB_EPOLL_CERR,  "-close epoll FD-");
    }
}


void FdValidate::dispose(const int fd) {
    std::lock_guard<std::mutex> lock(access_);

    const auto it = inuse_.find(fd);
    if (it == inuse_.end()) {
        return;
    }
    if (!is_valid(fd)) {
        inuse_.erase(it);
        return;
    }
    disposeBase(fd);

    inuse_.erase(it);
}


void FdValidate::add(const int client_fd, epoll_event& event)  noexcept
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == VB_NVALUE) {
        terminal(VB_EPOLL_CERR,  "-add new epoll FD-");
        close(client_fd);
    }

    std::lock_guard<std::mutex> lock(access_);
    inuse_[client_fd] = {false, steady_clock::now()};
}



void FdValidate::busy(const int client_fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    inuse_[client_fd] = {true, steady_clock::now()};

}


void FdValidate::unBusy(const int client_fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    if (const auto it = inuse_.find(client_fd); it != inuse_.end()) {
        it->second = {false, steady_clock::now()};;
    }
}

bool FdValidate::isBusy(const int client_fd) noexcept
{
    std::lock_guard<std::mutex> lock(access_);
    const auto it = inuse_.find(client_fd);
    return it != inuse_.end() && it->second.state;
}


void FdValidate::clearOldFd() {
    const auto now = steady_clock::now();

    for (auto it = inuse_.begin(); it != inuse_.end(); ) {
        if (auto duration = duration_cast<seconds>(now - it->second.last_active);
            duration.count() > TIMEOUT_LIMIT_SECONDS) {

            disposeBase(it->first);
            it = inuse_.erase(it); 
            } else {
                ++it;
            }
    }
}

void FdValidate::retryBusyMessages() {

    const auto queue = Server::getRetryQueue();

    for (auto it = queue->begin(); it != queue->end(); ++it ) {
        if(
            auto [key, value] = *it;
            !isBusy(key) && is_valid(key)
          ) {
            if (const bool result = Server::sendResponse(value, epoll_fd,key); !result) {
                terminal(VB_EPOLL_CERR, VB_SOCKET_FAIL);
            }
            disposeBase(key);
         }
        queue->erase(it);
    }
}

