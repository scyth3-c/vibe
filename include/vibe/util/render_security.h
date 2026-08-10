//
// Security knobs for the file-rendering family (readFile, readFileX,
// compose, render). Defaults keep the legacy behavior except where the
// legacy behavior was inherently unsafe (resource limits are always on).
//
//   router.configure({
//       .render = {
//           .root             = "public/",   // jail every render path
//           .allow_readfilex  = false,       // disable C++ templates
//           .run_timeout      = std::chrono::milliseconds{500},
//       },
//   });
//

#ifndef VIBE_RENDER_SECURITY_H
#define VIBE_RENDER_SECURITY_H

#include <chrono>
#include <cstddef>
#include <string>

namespace vibe {

    struct RenderSecurity {
        // Jail: every path handed to readFile/file/readFileX/compose/render
        // must resolve (symlinks included) inside this directory.
        // Empty = no jail (legacy behavior, NOT recommended for production).
        std::string root{};

        // Maximum bytes a single rendered file may occupy in memory.
        size_t max_file_bytes = 32UL * 1024UL * 1024UL;

        // Master switch for readFileX: C++ templates compile and execute
        // code, so deployments that do not need them should turn this off.
        bool allow_readfilex = true;

        // ---- readFileX sandbox ----
        std::chrono::milliseconds compile_timeout{15000}; // g++ wall clock
        std::chrono::milliseconds run_timeout{5000};      // template wall clock
        size_t run_memory_bytes   = 256UL * 1024UL * 1024UL; // RLIMIT_AS of the executed program
        size_t max_output_bytes   = 8UL * 1024UL * 1024UL;   // captured stdout cap
        size_t compile_cache_entries = 64;                   // binaries kept per code hash
    };

} // namespace vibe

#endif // VIBE_RENDER_SECURITY_H
