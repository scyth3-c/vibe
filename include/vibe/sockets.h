#ifndef VIBE_SOCKETS_HPP
#define VIBE_SOCKETS_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <memory>
#include <unistd.h>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <mutex>
#include "sysop/literals.h"
#include "configuration.hpp"

using std::string, std::shared_ptr, std::make_shared, std::vector, std::function;

class Engine {

   protected:

        std::mutex lock_guard;
        std::mutex response_guard;
        uint16_t PORT;
        shared_ptr 
                   <int> 
                         socket_id = nullptr,
                         state_receptor = nullptr,
                         address_len = make_shared<int>(sizeof(address)),
                         option_mame = make_shared<int>(0x1),
                         buffer_size = make_shared<int>(DEF_BUFFER_SIZE);
    public:

        sockaddr_in address{};
        explicit Engine(uint16_t);
        virtual ~Engine() = default;


    [[maybe_unused]] int
             setBuffer(int),
             setPort(uint16_t);

    [[nodiscard]] int getPort() const;

        virtual int on() = 0;
        [[maybe_unused]] virtual int Close() = 0;

};

struct SendData {
    int socket;
    std::string data;
};

class Server final : public Engine {

     shared_ptr<string> buffered_data;
     shared_ptr
               <int> static_sessions = make_shared<int>(10);
    int epoll_fd;

    std::vector<epoll_event> events;
    static shared_ptr<vector<std::pair<int, string>>> retryQueue;

  public:
     
     explicit Server(uint16_t const Port) : Engine(Port), epoll_fd(-1) {}
     Server() : Engine(DEFAULT_PORT), epoll_fd(-1) { }

     int on() override;
     int Close() override;

    [[maybe_unused]] [[nodiscard]] inline int getDescription() const {  return *socket_id;  }
    [[maybe_unused]]  shared_ptr<int> getSocketId() { return socket_id; }
    [[maybe_unused]]  void setSocketId(int const identity) { socket_id = std::make_shared<int>(identity); }

     void setSessions(int);
     [[nodiscard]] bool sendResponse(const string&, int client_descriptor) const;
     [[nodiscard]] bool queueResponseForSending(const string&, int client_descriptor) const;
     static int setNonblocking(const int&);
};



#endif // !VIBE_SOCKETS_HPP