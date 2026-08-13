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

#ifndef VERMELL_RENDER_SECURITY_H
#define VERMELL_RENDER_SECURITY_H

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace vermell {

    // C++ toolchain used to build readFileX templates. Fully configurable
    // from the router:
    //
    //   router.configure({
    //       .render = {
    //           .cpp = {
    //               .compiler = "/usr/bin/g++-12", // or bare "g++" / "" (auto)
    //               .standard = "c++20",
    //               .optimize = "-O2",
    //               .flags    = {"-I", "templates/", "-lm"},
    //           },
    //       },
    //   });
    //
    //   // or with the fluent setter:
    //   router.setCppToolchain({ .standard = "c++20" });
    //
    struct CppToolchain {
        // Compiler to invoke: absolute path, or a bare name searched in
        // /usr/bin, /bin and /usr/local/bin. Empty = auto-detect g++
        // (legacy behavior).
        std::string compiler{};

        std::string standard = "c++17"; // passed as -std=<standard>; empty = compiler default
        std::string optimize = "-O1";   // e.g. "-O2", "-g"; empty = no optimization flag

        bool hardening         = true;  // -fstack-protector-strong -D_FORTIFY_SOURCE=2 -s
        bool suppress_warnings = true;  // -w

        // Extra arguments appended after the built-in ones (later flags win
        // in gcc/clang): include paths, defines, libraries...
        std::vector<std::string> flags{};

        // Resource limits of the compiler process (RLIMIT_AS / RLIMIT_FSIZE).
        size_t memory_bytes    = 2UL * 1024UL * 1024UL * 1024UL;
        size_t file_size_bytes = 128UL * 1024UL * 1024UL;

        // Identity of the toolchain for the binary cache: the same template
        // built with two different toolchains yields two cache entries.
        [[nodiscard]] std::string fingerprint() const {
            std::string out = compiler;
            out += '\x1f' + standard + '\x1f' + optimize;
            out += hardening ? '1' : '0';
            out += suppress_warnings ? '1' : '0';
            for (const auto& flag : flags) {
                out += '\x1f';
                out += flag;
            }
            return out;
        }
    };

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

        // Toolchain the readFileX templates are compiled with.
        CppToolchain cpp{};
    };

} // namespace vermell

#endif // VERMELL_RENDER_SECURITY_H
