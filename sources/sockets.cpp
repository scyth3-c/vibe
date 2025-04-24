
#include "../include/vibe/sockets.h"

Engine::Engine(const uint16_t port) : PORT(port) {}

shared_ptr<vector<std::pair<int, string>>> Server::retryQueue = make_shared<vector<std::pair<int, string>>>();

int Server::Close() {
     if (close(*socket_id) < 0) {
               throw std::range_error("Failed to close socket");
     }
          return MG_OK;
}

int Engine::setPort(const uint16_t xPort) {
          PORT = xPort;
          if(PORT <= 0) throw std::range_error("Failed to set port");
          return MG_OK;
}

int Engine::getPort() const {

          if (PORT > 0)  {
               return PORT;
          }
               throw std::range_error("Failed to get port");
}



int Engine::setBuffer(int size) {
          if (buffer_size == nullptr) {
               buffer_size = make_shared<int>(size);
          }
          buffer_size = std::make_shared<int>(size);
          if(*buffer_size != size) throw std::range_error("filed to set buffer_size");
          return MG_OK;
}



void Server::setSessions(int max) {
          if (static_sessions == nullptr) {
               static_sessions = make_shared<int>(max);
               return;
          }
          static_sessions = std::make_shared<int>(max);
          if(*static_sessions != max) throw std::range_error("Failed to set sessions");
}

int Server::setNonblocking(const int& socket_id) {
        const int flags = fcntl(socket_id, F_GETFL, 0);
        if (flags == -1){
            return MG_ERROR;
        }
        if (fcntl(socket_id, F_SETFL, flags | O_NONBLOCK) < 0){
            return MG_ERROR;
        }
        return MG_OK;
}


int Server::on() {

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

bool Server::sendResponse(const string& _msg, const int client_descriptor) const
{

     if (const ssize_t bytes_sent = send(client_descriptor, _msg.c_str(), _msg.size(), MSG_NOSIGNAL); bytes_sent == -1) {

          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               return queueResponseForSending(_msg, client_descriptor);
          }

               terminal(VB_SOCKET_FAIL, strerror(errno));
               epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_descriptor, nullptr);
               close(client_descriptor);
               return false;

     } else if (bytes_sent < static_cast<ssize_t>(_msg.size())) {
          return queueResponseForSending(_msg.substr(bytes_sent), client_descriptor);
     }

     return true;
}

bool Server::queueResponseForSending(const string& remaining_data, int client_descriptor) const
{

     epoll_event event{};
     event.events = EPOLLIN | EPOLLOUT | EPOLLET;
     event.data.fd = client_descriptor;

     if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_descriptor, &event) == -1) {
          terminal(VB_EPOLL_CTL, strerror(errno));
          return false;
     }

     retryQueue->emplace_back(client_descriptor, remaining_data);

     return true;
}
