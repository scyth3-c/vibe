#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
using std::string;

constexpr auto VB_NVALUE = -1;
constexpr auto VB_OK = 0;

constexpr auto VB_EPOLL_RANGE =  "AN ERROR OCCURRED WHEN CREATION OF THE EPOLL_FD FILE DESCRIPTOR";
constexpr auto VB_EPOLL_CTL =    "AN ERROR OCCURRED WHEN EXECUTING EPOLL_CTL";
constexpr auto VB_EPOLL_CERR =   "THE FOLLOWING ERROR WAS FOUND WHEN EXECUTING THE EPOLL METHOD: ";

constexpr auto VB_SOCKET_FAIL =  "A PROBLEM OCCURRED WHEN READING THE SOCKET REQUEST, CHECK THE CONNECTION";
constexpr auto VB_SOCKET_SEND =  "A PROBLEM OCCURRED WHEN SEND TO CLIENT, CHECK THE CONNECTION";
constexpr auto VB_SOCKET_CLOSE = "A PROBLEM OCCURRED WHEN TRYING TO CLOSE THE CONNECTION WITH THE SOCKET: normally it is due to an error in the previous code";

constexpr auto VB_MAIN_THREAD =  "AN ERROR OCCURRED IN THE MAIN PROCESSING THREAD";

template<class...P>
auto terminal(P const&... args) -> void {
  ((std::cout<<"[ "<<args<<"]"<<std::endl),...);
}

struct notify_html {
      static std::string noPath() noexcept {
        return "Vibe: error no se puedo encontrar la ruta! <br/> Vibe: error cant get the path! ";
      }
      static std::string noFIle(const std::string& name) noexcept {
        return "Vibe: error no se puedo encontrar el archivo " + name + " <br/> Vibe: error cant get the file  " + name;
      }

      static std::string noSafe() noexcept {
        return "Vibe: error de sintaxis no <strong>'];'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>'];'</strong>  ";
      }

      static std::string noSafeData() noexcept {
        return "Vibe: error de sintaxis no <strong>']]'</strong>  <br/> Vibe: sintax error whiout closing the tag with <strong>']]'</strong>  ";
      }
};

struct notify {
  static std::string noPath(const string& path) noexcept {
    return  "Can't get path file. '" + path + "',  ";
  }
};

struct literals {
  static std::string noPath(const string& path) noexcept {
    return  "Can't get path file. '" + path + "',  ";
  }
};
//
//
// [[maybe_unused]] constexpr auto HTML = "text/html; charset=utf-8 ";
// constexpr auto JSON = "application/json ";
//
// struct WEB {
//
//   static constexpr const char* JSON = "application/json";
//   static constexpr const char* SERVER = "Vibe/1.5";
//
//   WEB() = delete;
//
//   static std::string json(const std::string& txt, const std::string& status = "200 OK") {
//     std::ostringstream response;
//     response
//         << "HTTP/1.1 " << status << "\r\n"
//         << "Server: " << SERVER << "\r\n"
//         << "Content-Type: " << JSON << "\r\n"
//         << "Content-Length: " << txt.size() << "\r\n"
//         << "Accept-Ranges: bytes\r\n"
//         << "Connection: close\r\n"
//         << "\r\n"  // Fin de headers
//         << txt;
//
//     return response.str();
//   }
//
//   static std::string custom(const std::string& txt,
//                             const std::string& type,
//                             const std::string& additional_headers,
//                             const std::string& status = "200 OK") {
//     std::ostringstream response;
//     response
//         << "HTTP/1.1 " << status << "\r\n"
//         << "Server: " << SERVER << "\r\n"
//         << "Content-Type: " << type << "\r\n"
//         << "Content-Length: " << txt.size() << "\r\n"
//         << "Accept-Ranges: bytes\r\n"
//         << additional_headers
//         << "Connection: close\r\n"
//         << "\r\n"  // Fin de headers
//         << txt;
//
//     return response.str();
//   }
// };


// [[maybe_unused]] constexpr auto HTTP_ERROR = "HTTP/1.1 400 BAD\n"
//                            "Server: Vibe/1.0\n"
//                            "Content-Type: application/json\n"
//                            "Content-Length: 25\n"
//                            "Accept-Ranges: bytes\n"
//                            "Connection: close\n"
//                            "\n"
//                            "error!";

#endif // ! NOTIFY_HPP