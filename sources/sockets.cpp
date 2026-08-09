#include <memory>
#include <poll.h>
#include <cerrno>

#include "../include/vibe/sockets.h"

Engine::Engine(const uint16_t port) : PORT(port) {}

int Server::Close() {
     try {   

          if (close(*socket_id) < 0) {
               throw std::range_error("Failed to close socket");
          }
          return MG_OK;
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          return MG_ERROR;
     }
}

int Engine::setPort(const uint16_t xPort) {
     try {
          PORT = xPort;
          if(PORT <= 0) throw std::range_error("Failed to set port");
          return MG_OK;
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          return MG_ERROR;
     }
}

int Engine::getPort() const {
     try {
          if (PORT > 0)  {
               return PORT;
          }
          else {
               throw std::range_error("Failed to get port");
          }
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          return MG_ERROR;
     }
}



int Engine::setBuffer(int size) {
     try {
          if (buffer_size == nullptr) {
               buffer_size = make_shared<int>(size);
          }
          buffer_size = std::make_shared<int>(size);
          if(*buffer_size != size) throw std::range_error("filed to set buffer_size");
          return MG_OK;
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          return MG_ERROR;
     }
}



void Server::setSessions(int max) {
     try {
          if (static_sessions == nullptr) {
               static_sessions = make_shared<int>(max);
               return;
          }
          static_sessions = std::make_shared<int>(max);
          if(*static_sessions != max) throw std::range_error("Failed to set sessions");
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
     }
}

int Server::setNonblocking(const int& socket_id) {
        int flags = fcntl(socket_id, F_GETFL, 0);
        if (flags == -1){
            return MG_ERROR;
        }
        if (fcntl(socket_id, F_SETFL, flags | O_NONBLOCK) < 0){
            return MG_ERROR;
        }
        return MG_OK;
}


int Server::on() {
     try {

         socket_id = make_shared<int>(socket(DOMAIN, TYPE, PROTOCOL));
         if (*socket_id == MG_ERROR) {
             throw std::range_error("Failed to create domain socket");
         }

         if (setsockopt(*socket_id,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        &*option_mame,
                        sizeof(*option_mame)) != 0x0) {
             throw std::range_error("Failed to set socket options");
         }

         // Best effort: allows several sockets on the same port when available.
         setsockopt(*socket_id,
                    SOL_SOCKET,
                    SO_REUSEPORT,
                    &*option_mame,
                    sizeof(*option_mame));

         if(setNonblocking(*socket_id) == MG_ERROR)
             throw std::runtime_error("Failed to set nonblocking");

         address.sin_family = AF_INET;
         address.sin_addr.s_addr = INADDR_ANY;
         address.sin_port = htons(PORT);

         if (bind(*socket_id, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
               throw std::range_error("Failed to bind socket");
          }
          const int backlog = (static_sessions != nullptr && *static_sessions > 0)
                                  ? *static_sessions
                                  : SOMAXCONN;
          if (listen(*socket_id, backlog) < 0x0) {
               throw std::range_error("Failed to listen on socket");
           }

         return MG_OK;
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          return MG_ERROR;
     }
}

void Server::getResponseProcessing() {
    try {
        string base;
        vector<char> buffer;
        buffer.resize(*buffer_size);

        const auto total_bytes = read(*socket_id, buffer.data(), *buffer_size);

        for (auto it = 0; it <= total_bytes; it++) {
            if (buffer[it] == 0)
                break;
            if(buffer[it] == UnCATCH_ERROR_CH)
                continue;
            if (buffer[it] == 10)
                continue;
            base += buffer[it];
        }
        if(base.empty()) throw std::range_error("response is empty");
        buffereOd_data = make_shared<string>(base);
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; }
}

void Server::setResponse(const std::array<char,DEF_BUFFER_SIZE> &buffer) {
     if( std::string raw(buffer.begin(), buffer.size()) ; !raw.empty()) {
          buffereOd_data = make_shared<string>(raw);
     }
}

void Server::setResponse(const string &data) {
     if (!data.empty()) {
          buffereOd_data = make_shared<string>(data);
     }
}

void Server::sendResponse(const string& _msg) const {
     if (socket_id == nullptr || _msg.empty())
          return;

     const int fd = *socket_id;
     size_t sent_total = 0;

     // The fd is owned exclusively by the calling worker (it was removed from
     // epoll before dispatch), so we write directly: no shared epoll_wait and
     // MSG_NOSIGNAL to avoid SIGPIPE killing the process when the peer is gone.
     while (sent_total < _msg.size()) {

          const ssize_t bytes_send = send(fd, _msg.data() + sent_total, _msg.size() - sent_total, MSG_NOSIGNAL);

          if (bytes_send > 0) {
               sent_total += static_cast<size_t>(bytes_send);
               continue;
          }

          if (bytes_send == -1 && errno == EINTR)
               continue;

           if (bytes_send == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                pollfd pfd{};
                pfd.fd = fd;
                pfd.events = POLLOUT;

                if (poll(&pfd, 1, write_timeout_ms) > 0 && (pfd.revents & POLLOUT))
                     continue;
           }

          break; // real error or timeout: give up, the caller closes the fd
     }
}

