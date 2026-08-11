#ifndef VIBE_SOCKETS_HPP
#define VIBE_SOCKETS_HPP


#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>


#include <memory>
#include <unistd.h>
#include <string>
#include <sstream>
#include <cstring>
#include <functional>

#include <iostream>
#include <vector>
#include <array>
#include <mutex>
#include <charconv>
#include <chrono>
#include <string_view>

#include "http/response.hpp"

using std::string;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using std::function;

constexpr uint16_t DEFAULT_PORT = 0xBB8;

constexpr int DOMAIN = AF_INET;
constexpr int TYPE = SOCK_STREAM;
constexpr int PROTOCOL = 0;

constexpr int MG_ERROR = -0x1;
constexpr int MG_OK = 0x0;
[[maybe_unused]] constexpr int MG_CONFUSED = 0x1;

constexpr int DEF_BUFFER_SIZE = 0x400;
constexpr int UnCATCH_ERROR_CH = -0x42;

[[maybe_unused]] constexpr auto SOCK_ERR = "_ERROR";

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

        struct sockaddr_in address{};
        explicit Engine(uint16_t);
        virtual ~Engine() = default;


    [[maybe_unused]] int
             setBuffer(int),
             setPort(uint16_t);

    [[nodiscard]] int getPort() const;

        virtual int on() = 0;
        virtual void getResponseProcessing() = 0;

        [[maybe_unused]] virtual int Close() = 0;
        [[nodiscard]] virtual string getResponse() const = 0;
};

struct SendData {
    int socket;
    std::string data;
};

class Server final : public Engine {
  private:

     shared_ptr<string> buffereOd_data;
     shared_ptr
               <int> static_sessions = make_shared<int>(10);
     int epoll_fd, notices;

     int write_timeout_ms = 5000;

     std::vector<epoll_event> events;

  public:

     explicit Server(uint16_t const Port) : Engine(Port), epoll_fd(-1), notices(-1) {}
     Server() : Engine(DEFAULT_PORT), epoll_fd(-1), notices(-1){}

    void getResponseProcessing() override;
     int on() override;
     int Close() override;

     [[maybe_unused]] [[nodiscard]] inline int getDescription() const {  return *socket_id;  }
     [[maybe_unused]] inline shared_ptr<int> getSocketId() { return socket_id; }
     [[maybe_unused]] inline void setSocketId(int const identity) { socket_id = std::make_shared<int>(identity); }

     void setSessions(int);
     // Inactivity timeout while writing the response to the client.
     inline void setWriteTimeout(const std::chrono::milliseconds timeout) noexcept {
          write_timeout_ms = static_cast<int>(timeout.count());
     }
     void sendResponse(const string&) const;
     void setResponse(const std::array<char, DEF_BUFFER_SIZE> &buffer);
     void setResponse(const string &data);

     inline void setEpollEvents(std::vector<epoll_event> const &e){events = e;}
     inline void setEpollfd(int const arg) noexcept { epoll_fd = arg; }
     inline void setNotices(int const arg) noexcept { notices = arg;  }

     [[nodiscard]] inline std::vector<epoll_event> getEpollEvents() const {return events; }
     [[nodiscard]] inline int getEpollfd() const {return epoll_fd;}
     [[nodiscard]] inline int getNotices() const {return notices;}


     static int setNonblocking(const int&);

     [[nodiscard]] inline string getResponse()  const override {
            return buffereOd_data != nullptr ? *buffereOd_data : string{};
       }
};


#endif // !VIBE_SOCKETS_HPP