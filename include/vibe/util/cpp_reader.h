//
// readFileX: renders ".html"-style templates with an embedded C++ block:
//
//     <body>
//       $ std::cout << "hello" << std::endl; $
//     </body>
//
// Hardening over the legacy implementation:
//   - The scanner cannot run out of bounds or touch uninitialized memory
//     (empty files, missing/loose '$' are served as plain text).
//   - Compilation and execution run sandboxed: private mkdtemp workspace,
//     scrubbed environment, no inherited file descriptors, rlimits on
//     CPU/memory/output and wall-clock timeouts enforced with SIGKILL.
//   - The toolchain (compiler, standard, optimization, hardening, extra
//     flags and compiler resource limits) is fully configurable through
//     vibe::RenderSecurity::cpp — see router.configure({ .render = { .cpp
//     = {...} } }) or router.setCppToolchain({...}).
//   - Compiled binaries are cached under a private per-user directory keyed
//     by the SHA-256 of the generated source plus the toolchain fingerprint,
//     so steady-state requests do not pay for g++ (and a compile flood
//     cannot stall the worker pool).
//   - Compiler/program diagnostics are logged server-side; the client only
//     receives a generic error page.
//

#ifndef CPP_READER_HPP
#define CPP_READER_HPP

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <unistd.h>

#include "local_utility.h"
#include "nterminal.h"
#include "notify.h"
#include "render_security.h"
#include "secure_render.h"
#include "sysprocess.h"

using std::string;
using neosys::process;
using neosys::RunOptions;

class CppReader {
public:
    CppReader() = default;

    static std::pair<string, string> processing(const string& path,
                                                const vibe::RenderSecurity& sec = {}) {
        if (!sec.allow_readfilex)
            return {"Vibe: C++ templates are disabled on this server", "403"};

        if (!sec.root.empty() && !vibe::srender::is_within(sec.root, path))
            return {notify::noPath(path), "403"};

        auto read = vibe::srender::read_bounded(path, sec.max_file_bytes);
        if (read.err != vibe::srender::ReadErr::Ok)
            return {notify::noPath(path), vibe::srender::status_of(read.err)};

        const string raw_html = std::move(read.data);

        // No code block (or a lone '$'): serve the file verbatim instead of
        // running with uninitialized coordinates like the legacy code did.
        const auto segment = locate_code(raw_html);
        if (!segment)
            return {raw_html, "200"};

        try {
            TempDir work = make_temp_dir();
            if (work.path.empty())
                return {"Vibe: cannot allocate a sandbox workspace", "500"};

            const string code = string(BASE)
                    + raw_html.substr(segment->first + 1, segment->second - segment->first - 1)
                    + CODE_END;

            string error;
            const string binary = get_or_compile(code, work, sec, error);
            if (binary.empty()) {
                if (!error.empty())
                    std::cerr << "vibe readFileX compile error: " << error << std::endl;
                return {"Vibe: the template could not be compiled", "400"};
            }

            const string out_file = work.path + "/out.txt";

            RunOptions run_opts;
            run_opts.timeout         = sec.run_timeout;
            run_opts.cpu_seconds     = static_cast<rlim_t>(sec.run_timeout.count() / 1000 + 1);
            run_opts.memory_bytes    = sec.run_memory_bytes;
            run_opts.file_size_bytes = sec.max_output_bytes;
            run_opts.max_processes   = 32; // no fork bombs from a template
            run_opts.work_dir        = work.path.c_str();
            run_opts.new_session     = true;

            const std::vector<const char*> execute = {binary.c_str(), nullptr};
            if (process::run_command(execute, out_file, run_opts) == VB_NVALUE) {
                std::cerr << "vibe readFileX: template execution failed or timed out" << std::endl;
                return {"Vibe: the template execution failed or timed out", "400"};
            }

            auto out = vibe::srender::read_bounded(out_file, sec.max_output_bytes);
            if (out.err == vibe::srender::ReadErr::TooLarge)
                return {"Vibe: the template output exceeded the allowed size", "400"};

            return {raw_html.substr(0, segment->first)
                        + out.data
                        + raw_html.substr(segment->second + 1),
                    "200"};
        } catch (const std::exception &e) {
            std::cerr << "vibe readFileX internal error: " << e.what() << std::endl;
            return {"Vibe: internal error while rendering the template", "500"};
        }
    }

private:
    // ---------------- template scanning ----------------

    // Returns the positions of the opening and closing '$' (inclusive), or
    // nullopt when the file holds no complete code block.
    [[nodiscard]] static std::optional<std::pair<size_t, size_t>>
    locate_code(const string& raw) noexcept {
        const size_t open = raw.find(CODE_LOCATE);
        if (open == string::npos)
            return std::nullopt;
        const size_t close = raw.find(CODE_LOCATE, open + 1);
        if (close == string::npos)
            return std::nullopt;
        return std::pair{open, close};
    }

    // ---------------- private workspace ----------------

    struct TempDir {
        string path{};
        TempDir() = default;
        TempDir(const TempDir&) = delete;
        TempDir& operator=(const TempDir&) = delete;
        TempDir(TempDir&& other) noexcept : path(std::move(other.path)) { other.path.clear(); }
        TempDir& operator=(TempDir&&) = delete;
        ~TempDir() {
            if (!path.empty()) {
                std::error_code ec;
                std::filesystem::remove_all(path, ec);
            }
        }
    };

    [[nodiscard]] static string tmp_base() {
        if (const char* env = std::getenv("TMPDIR"); env != nullptr && env[0] != '\0')
            return env;
        return "/tmp";
    }

    [[nodiscard]] static TempDir make_temp_dir() {
        TempDir dir;
        string pattern = tmp_base() + "/vibe-x-XXXXXX";
        std::vector<char> buf(pattern.begin(), pattern.end());
        buf.push_back('\0');
        if (mkdtemp(buf.data()) != nullptr) // mkdtemp creates it with mode 0700
            dir.path = buf.data();
        return dir;
    }

    // ---------------- compiler location ----------------

    // Legacy auto-detection: the first g++ found in the usual install dirs.
    [[nodiscard]] static string detect_compiler() {
        static const string found = [] {
            std::error_code ec;
            for (const char* candidate : {"/usr/bin/g++", "/bin/g++", "/usr/local/bin/g++"}) {
                if (std::filesystem::is_regular_file(candidate, ec) && !ec)
                    return string(candidate);
            }
            return string{};
        }();
        return found;
    }

    // Resolves the configured toolchain compiler to an absolute path (the
    // sandboxed execve does not search PATH): absolute paths are used
    // as-is, bare names are searched in the usual install dirs and an empty
    // value falls back to g++ auto-detection.
    [[nodiscard]] static string resolve_compiler(const vibe::CppToolchain& tc) {
        if (tc.compiler.empty())
            return detect_compiler();

        const auto usable = [](const string& candidate) {
            std::error_code ec;
            return std::filesystem::is_regular_file(candidate, ec) && !ec
                   && access(candidate.c_str(), X_OK) == 0;
        };

        if (tc.compiler.find('/') != string::npos)
            return usable(tc.compiler) ? tc.compiler : string{};

        for (const char* dir : {"/usr/bin", "/bin", "/usr/local/bin"}) {
            const string candidate = string(dir) + "/" + tc.compiler;
            if (usable(candidate))
                return candidate;
        }
        return {};
    }

    // ---------------- binary cache ----------------

    struct Cache {
        std::mutex mtx;
        std::unordered_map<string, string> entries; // sha256 -> binary path
        string dir{};
        bool usable = false;

        Cache() {
            const string candidate = tmp_base() + "/vibe-cx-" + std::to_string(getuid());
            std::error_code ec;
            std::filesystem::create_directory(candidate, ec); // ignores EEXIST
            struct stat st{};
            if (stat(candidate.c_str(), &st) == 0
                && S_ISDIR(st.st_mode)
                && st.st_uid == getuid()
                && (st.st_mode & 077) == 0) {
                dir = candidate;
                usable = true;
            } else if (stat(candidate.c_str(), &st) == 0 && S_ISDIR(st.st_mode)
                       && st.st_uid == getuid()) {
                // Existed with loose permissions: tighten instead of giving up.
                if (chmod(candidate.c_str(), 0700) == 0) {
                    dir = candidate;
                    usable = true;
                }
            }
        }
    };

    [[nodiscard]] static Cache& cache() {
        static Cache instance;
        return instance;
    }

    // Builds the compiler argv from the configured toolchain. The storage
    // vector owns every string so the c_str() pointers stay valid until
    // run_command() returns.
    [[nodiscard]] static std::vector<string>
    build_compile_args(const vibe::CppToolchain& tc, const string& cxx,
                       const string& source, const string& binary) {
        std::vector<string> args;
        args.reserve(12 + tc.flags.size());

        args.push_back(cxx);
        if (!tc.standard.empty())
            args.push_back("-std=" + tc.standard);
        if (!tc.optimize.empty())
            args.push_back(tc.optimize);
        if (tc.hardening) {
            args.emplace_back("-fstack-protector-strong");
            args.emplace_back("-D_FORTIFY_SOURCE=2");
            args.emplace_back("-s");
        }
        if (tc.suppress_warnings)
            args.emplace_back("-w");
        // User flags go last: in gcc/clang later options win (-O, -std, ...).
        for (const auto& flag : tc.flags)
            args.push_back(flag);

        args.push_back(source);
        args.emplace_back("-o");
        args.push_back(binary);
        return args;
    }

    // Returns the path of a ready-to-run binary for `code`, or an empty
    // string on failure (diagnostics land in `error`).
    [[nodiscard]] static string get_or_compile(const string& code,
                                               const TempDir& work,
                                               const vibe::RenderSecurity& sec,
                                               string& error) {
        const vibe::CppToolchain& tc = sec.cpp;

        const string cxx = resolve_compiler(tc);
        if (cxx.empty()) {
            error = "no usable C++ compiler found (configured: '"
                    + (tc.compiler.empty() ? string("auto-detect") : tc.compiler) + "')";
            return {};
        }

        // The toolchain is part of the identity of the binary: the same
        // source built with another compiler/flag set is another entry.
        const string key = vibe::srender::sha256_hex(
            code + '\x1f' + cxx + '\x1f' + tc.fingerprint());

        Cache& store = cache();
        {
            std::lock_guard<std::mutex> lock(store.mtx);
            if (const auto it = store.entries.find(key);
                it != store.entries.end() && std::filesystem::exists(it->second))
                return it->second;
        }

        const bool cacheable = store.usable
                               && (store.entries.size() < sec.compile_cache_entries
                                   || sec.compile_cache_entries == 0);
        const string binary = cacheable
                                  ? work.path + "/tpl.bin"   // renamed into the cache on success
                                  : work.path + "/once.bin"; // cache full/off: ephemeral
        const string source = work.path + "/tpl.cpp";
        const string log    = work.path + "/compile.log";

        {
            std::ofstream out(source, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                error = "cannot write the translation unit";
                return {};
            }
            out << code;
        }

        const std::vector<string> storage = build_compile_args(tc, cxx, source, binary);
        std::vector<const char*> compile;
        compile.reserve(storage.size() + 1);
        for (const auto& arg : storage)
            compile.push_back(arg.c_str());
        compile.push_back(nullptr);

        RunOptions compile_opts;
        compile_opts.timeout         = sec.compile_timeout;
        compile_opts.cpu_seconds     = static_cast<rlim_t>(sec.compile_timeout.count() / 1000 + 1);
        compile_opts.memory_bytes    = tc.memory_bytes;
        compile_opts.file_size_bytes = tc.file_size_bytes;
        compile_opts.work_dir        = work.path.c_str();
        compile_opts.new_session     = true;

        if (process::run_command(compile, log, compile_opts) == VB_NVALUE) {
            auto details = vibe::srender::read_bounded(log, 4UL * 1024UL);
            error = std::move(details.data);
            return {};
        }

        if (!cacheable)
            return binary;

        // Publish atomically; a concurrent request compiling the same code
        // wins the race and its binary is reused.
        const string cached_path = store.dir + "/" + key;
        std::lock_guard<std::mutex> lock(store.mtx);
        std::error_code ec;
        if (!std::filesystem::exists(cached_path, ec)) {
            std::filesystem::rename(binary, cached_path, ec);
            if (ec) {
                std::filesystem::remove(cached_path, ec);
                ec.clear();
                std::filesystem::rename(binary, cached_path, ec);
                if (ec)
                    return binary; // keep the ephemeral copy, just uncached
            }
            std::filesystem::permissions(cached_path,
                                         std::filesystem::perms::owner_all,
                                         ec);
        } else {
            std::filesystem::remove(binary, ec);
        }
        store.entries[key] = cached_path;
        return cached_path;
    }
};

#endif // ! CPP_READER_HPP
