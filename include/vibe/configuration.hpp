#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "abstract.hpp"
#include <sys/socket.h>

using enums::neo;


// router epoll configuration
constexpr int BUFFER = neo::eSize::BUFFER;
constexpr int SESSION = neo::eSize::SESSION;
constexpr int INIT_MAX_EVENTS = 256;
constexpr int CLOCK_SPEED = 5;
constexpr int TIMEOUT_LIMIT_SECONDS = 1800;
constexpr int STEP_TO_KILL = 1; // n request  before kill process for router.listenOne()
constexpr int SLEEP_AFTER_KILL_IN_MS = 20; // timeout after killing the process using .listenOne() in MS

// SOCKETS.H CONFIGURATION AREA
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
// END FOR SOCKET CONFIG


#endif //CONFIGURATION_H