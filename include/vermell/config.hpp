//
// Centralized server configuration. Every knob of the request/response
// pipeline is parametrized here; defaults preserve the legacy behavior.
//
//   router.configure({
//       .max_request_size = 64 * 1024 * 1024, // heavy uploads
//       .read_timeout      = std::chrono::seconds{30},
//       .threads           = 8,
//   });
//

#ifndef VERMELL_CONFIG_HPP
#define VERMELL_CONFIG_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <sys/socket.h>

#include "util/enums.h"
#include "util/render_security.h"

namespace vermell {

    struct Config {
        // ---- network ----
        uint16_t port = 0; // listening port; 0 = default/keep current (DEF_PORT)
        int backlog = SOMAXCONN;                     // pending connections queue of listen()
        int buffer_size = enums::neo::eSize::BUFFER; // legacy socket buffer size

        // ---- request reading ----
        // Inactivity timeout between chunks: raise it for heavy uploads on
        // slow networks (e.g. a large image arriving in many TCP segments).
        std::chrono::milliseconds read_timeout{5000};
        // Inactivity timeout while writing the response back to the client.
        std::chrono::milliseconds write_timeout{5000};
        // Hard limit for a whole request (headers + body). Requests bigger
        // than this are rejected with 413 Payload Too Large.
        size_t max_request_size = 4UL * 1024UL * 1024UL;
        // Bytes read per recv() call.
        size_t read_chunk = 16UL * 1024UL;

        // ---- concurrency / epoll ----
        size_t threads = 0; // worker threads; 0 = auto (hardware_concurrency)
        int max_events = 1024;                       // epoll event batch size
        // Queued tasks before the dispatcher blocks (backpressure). 0 = auto:
        // max(1024, threads * 256), enough to absorb an epoll batch burst.
        size_t max_queue_size = 0;
        // Hard cap on simultaneously open client connections. 0 = unlimited
        // (legacy). Setting a bound is the blunt DoS wall against
        // connection-flood / slowloris style exhaustion.
        size_t max_connections = 0;
        std::chrono::milliseconds epoll_timeout{1000}; // listen loop wake-up period

        // ---- file rendering hardening (readFile / readFileX / compose / render) ----
        RenderSecurity render{};
    };

} // namespace vermell

#endif // VERMELL_CONFIG_HPP
