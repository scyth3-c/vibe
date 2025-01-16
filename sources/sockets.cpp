#include <memory>

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


         if ((socket_id = make_shared<int>(
                 socket(DOMAIN, TYPE, PROTOCOL))) == nullptr) {
             throw std::range_error("Failed to create domain socket");
         }

         if (setsockopt(*socket_id,
                        SOL_SOCKET,
                        SO_REUSEADDR |
                        SO_REUSEPORT,
                        &*option_mame,
                        sizeof(*option_mame)) != 0x0) {
             throw std::range_error("Failed to set socket options");
         }

         if(setNonblocking(*socket_id) == MG_ERROR)
             throw std::runtime_error("Failed to set nonblocking");

         address.sin_family = AF_INET;
         address.sin_addr.s_addr = INADDR_ANY;
         address.sin_port = htons(PORT);

         unlink("127.0.0.1");
         if (bind(*socket_id, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
               throw std::range_error("Failed to bind socket");
          }
         if (listen(*socket_id, 3) < 0x0) {
              throw std::range_error("Failed to listen on socket");
          }

         return MG_OK;
     }
     catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          lock_guard.unlock();
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

void Server::sendResponse(const string& _msg) const {
     SendData data;
     data.socket = *socket_id;
     data.data = _msg;

     epoll_event event{};
     event.events = EPOLLOUT | EPOLLET;
     event.data.ptr = &data;

     if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, *socket_id, &event) == -1) {
          std::cerr << "epoll_ctl " << strerror(errno) << std::endl;
     }

     std::array<epoll_event, 1> events{};

     if(const int n = epoll_wait(epoll_fd, events.data(), 1, -1) ; n == -1) {
          std::cerr << "epoll_wait " << strerror(errno) << std::endl;
          return;
     }
     
     bool hasEPOLLOUT = (events[0].events & EPOLLOUT) != 0;

     if(hasEPOLLOUT) {

          if ( const ssize_t bytes_send =  send(data.socket, data.data.c_str(), data.data.size(), 0) ; bytes_send == -1) {

               std::cerr << "Error: Sending data" << std::endl;
               epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data.socket, nullptr);
               close(data.socket);

          } else if(bytes_send == 0) {
               epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data.socket, nullptr);
               close(data.socket);
          } else {
               epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data.socket, &events[0]);
          }
     }



}

