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


[[maybe_unused]] constexpr auto HTML = "text/html; charset=utf-8 ";
constexpr auto JSON = "application/json ";

struct [[maybe_unused]] WEB {

    explicit WEB()= default;

    [[maybe_unused]] static string json(const string& _txt, const string& status="200 OK") {
        return vibe::http::Response{}
            .status(status_code_of(status))
            .type("application/json")
            .body(_txt)
            .str();
     }


    [[maybe_unused]] static string custom(const string& _txt, const string& type, const string& headers,  const string& status="200 OK"){

        vibe::http::Response response;
        response.status(status_code_of(status)).type(type);

        size_t pos = 0;
        while (pos < headers.size()) {
            const size_t eol = headers.find('\n', pos);
            std::string_view line = std::string_view(headers).substr(
                pos, eol == string::npos ? eol : eol - pos);

            if (const size_t colon = line.find(':'); colon != std::string_view::npos && colon > 0) {
                std::string_view value = line.substr(colon + 1);
                if (!value.empty() && value.front() == ' ')
                    value.remove_prefix(1);
                if (!value.empty() && value.back() == '\r')
                    value.remove_suffix(1);
                response.set(line.substr(0, colon), value);
            }

            if (eol == string::npos)
                break;
            pos = eol + 1;
        }

        return response.body(_txt).str();
     }

private:
    // Legacy signature passes the status as a string ("200", "200 OK").
    static int status_code_of(const string& status) {
        int code = 200;
        const char* first = status.data();
        const char* last  = first + status.size();
        if (const auto [ptr, ec] = std::from_chars(first, last, code);
            ec == std::errc{} && ptr != first)
            return code;
        return 200;
    }
};

template<class...P>
struct HEADERS_MG {
    HEADERS_MG()= default;
    [[maybe_unused]] HEADERS_MG(std::initializer_list<P...>list): body(list) {}
    vector<string> body;
    [[maybe_unused]] string generate(){
        string response;
        for (auto &it : body) {
            response += it + "\n";
        }
        return response;
    }
};

using Headers = HEADERS_MG<string>;

[[maybe_unused]] constexpr auto HTTP_ERROR = "HTTP/1.1 400 BAD\n"
                           "Server: Vibe/1.0\n"
                           "Content-Type: application/json\n"
                           "Content-Length: 25\n"
                           "Accept-Ranges: bytes\n"
                           "Connection: close\n"
                           "\n"
                           "error!";


#endif // !VIBE_SOCKETS_HPP