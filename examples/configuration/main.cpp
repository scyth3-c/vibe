#include <vibe/vibe.h>

//
// Server configuration: every knob of the request/response pipeline lives
// in vibe::Config (include/vibe/config.hpp). Pass it whole with
// router.configure({...}) using designated initializers, or use the
// chainable setters. Defaults preserve the legacy behavior.
//
int main() {

    Router router;
    router.setPort(8080);

    router.configure({
        // ---- network ----
        // .port = 8080,          // 0 (default) keeps the port already set
        .backlog           = SOMAXCONN, // pending connections queue of listen()
        .buffer_size       = 2048,      // legacy socket buffer size

        // ---- request reading ----
        .read_timeout      = std::chrono::seconds{30}, // inactivity between chunks (raise it for heavy uploads)
        .write_timeout     = std::chrono::seconds{10}, // inactivity while writing the response
        .max_request_size  = 16UL * 1024UL * 1024UL,   // whole request cap; bigger => 413 Payload Too Large
        .read_chunk        = 32UL * 1024UL,            // bytes read per recv() call

        // ---- concurrency / epoll ----
        .threads           = 4,    // worker threads; 0 = auto (hardware_concurrency)
        .max_events        = 1024, // epoll event batch size
        .max_queue_size    = 512,  // queued tasks before the dispatcher blocks (backpressure); 0 = auto
        .epoll_timeout     = std::chrono::milliseconds{1000}, // listen loop wake-up period

        // ---- file rendering hardening (readFile / file / readFileX / compose / render) ----
        .render = {
            .root             = "./", // jail: no rendered path escapes this directory (empty = no jail)
            .max_file_bytes   = 32UL * 1024UL * 1024UL,
            .allow_readfilex  = true, // C++ templates compile & run code; turn off when unused
            .compile_timeout  = std::chrono::milliseconds{15000},
            .run_timeout      = std::chrono::milliseconds{5000},
            .run_memory_bytes = 256UL * 1024UL * 1024UL,
            .max_output_bytes = 8UL * 1024UL * 1024UL,

            // ---- readFileX toolchain: how the embedded C++ is compiled ----
            .cpp = {
                .compiler = "",        // absolute path or bare name ("g++-12"); empty = auto-detect g++
                .standard = "c++20",   // passed as -std=<standard>; empty = compiler default
                .optimize = "-O2",     // e.g. "-O0", "-g"; empty = no optimization flag
                .hardening         = true, // -fstack-protector-strong -D_FORTIFY_SOURCE=2 -s
                .suppress_warnings  = true, // -w
                .flags    = {"-I", "templates/includes"}, // appended last: defines, -I, -l...
                // resource limits of the compiler process itself:
                .memory_bytes    = 2UL * 1024UL * 1024UL * 1024UL,
                .file_size_bytes = 128UL * 1024UL * 1024UL,
            },
        },
    });

    // Same settings with the chainable setters:
    //
    //   router.setReadTimeout(std::chrono::seconds{30})
    //         .setWriteTimeout(std::chrono::seconds{10})
    //         .setMaxRequestSize(16UL * 1024UL * 1024UL)
    //         .setReadChunkSize(32UL * 1024UL)
    //         .setThreads(4)
    //         .setMaxEvents(1024)
    //         .setMaxQueueSize(512)
    //         .setBacklog(SOMAXCONN)
    //         .setBufferSize(2048);
    //
    // and the readFileX toolchain has its own setter:
    //
    //   router.setCppToolchain({ .standard = "c++20", .optimize = "-O2" });

    router.get("/",{[&](Query &web) {

        // the active configuration is readable at runtime:
        web.send("worker threads: " + std::to_string(router.config().threads));
    }});


    router.listen();
}
