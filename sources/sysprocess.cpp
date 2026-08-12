//
// Created by owl on 30/07/24.
//

#include "../include/vibe/util/sysprocess.h"
#include <algorithm>
#include <iostream>
#include <iterator>

#include "../include/vibe/util/nterminal.h"

const char* const neosys::process::log_path = "log_cv.log";
const std::string neosys::process::path = "PATH=$PATH:/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin";


std::mt19937& neosys::process::get_rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

unsigned long neosys::process::random()  {
    static std::uniform_int_distribution<std::mt19937::result_type> dist;
    return dist(get_rng());
}

namespace {

    // Applies one rlimit in the child; failures abort the exec so a
    // half-sandboxed program never runs.
    bool apply_limit(const int resource, const rlim_t value) {
        if (value == 0)
            return true;
        const rlimit lim{value, value};
        return setrlimit(resource, &lim) == 0;
    }

    void child_setup(const neosys::RunOptions &opts) {
        if (opts.new_session)
            setsid(); // own process group: the parent can SIGKILL all of it

        umask(077);
        const rlimit no_core{0, 0};
        setrlimit(RLIMIT_CORE, &no_core);

        if (!apply_limit(RLIMIT_CPU,   opts.cpu_seconds)     ||
            !apply_limit(RLIMIT_AS,    opts.memory_bytes)    ||
            !apply_limit(RLIMIT_FSIZE, opts.file_size_bytes) ||
            !apply_limit(RLIMIT_NPROC, opts.max_processes))
            _Exit(127);

        if (opts.work_dir != nullptr && chdir(opts.work_dir) != 0)
            _Exit(127);

        // Do not leak server fds (listening socket, epoll, files) into the
        // child: everything above stderr goes away. sysconf() may return -1
        // ("indeterminate"): fall back to a sane bound instead of skipping.
        long open_max = sysconf(_SC_OPEN_MAX);
        if (open_max < 0)
            open_max = 4096;
        if (open_max > 4096)
            open_max = 4096;
        for (long fd = 3; fd < open_max; ++fd)
            close(static_cast<int>(fd));
    }

} // namespace

int neosys::process::run_command(const std::vector<const char*> &args,
                                 const std::string& _path,
                                 const RunOptions& opts) {
    int status;

    if (args.empty() || !static_cast<bool>(args[0])) {
        return VB_NVALUE;
    }

    if (const pid_t pid = fork(); pid == VB_NVALUE) {
        return VB_NVALUE;
    } else if (pid != 0) {
        // ---- parent ----
        if (opts.timeout.count() <= 0) {
            // Legacy: block until the child finishes.
            pid_t retval;
            while ((retval = waitpid(pid, &status, 0)) != VB_NVALUE) {
                if (retval == pid) break;
                if (errno == EINTR) continue;
            }
            if (retval == VB_NVALUE || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                return VB_NVALUE;
            }
            return 0;
        }

        // Bounded wait: poll the child and kill it once the budget burns out.
        const auto deadline = std::chrono::steady_clock::now() + opts.timeout;
        for (;;) {
            const pid_t retval = waitpid(pid, &status, WNOHANG);
            if (retval == pid) {
                return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : VB_NVALUE;
            }
            if (retval == VB_NVALUE) {
                if (errno == EINTR)
                    continue;
                return VB_NVALUE;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                if (opts.new_session)
                    kill(-pid, SIGKILL); // the whole group first
                kill(pid, SIGKILL);      // fallback if setsid() failed
                while (waitpid(pid, &status, 0) == VB_NVALUE && errno == EINTR)
                    ;
                return VB_NVALUE;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    } else {
        // ---- child ----
        child_setup(opts);

        // No heap allocations between fork() and execve(): malloc is not
        // async-signal-safe and another worker thread may hold the arena
        // lock at the moment of the fork (deadlock). A stack array is enough.
        constexpr size_t MAX_ARGS = 256;
        if (args.size() >= MAX_ARGS)
            _Exit(127);

        std::array<char*, MAX_ARGS> argv{};
        for (size_t i = 0; i < args.size(); ++i)
            argv[i] = const_cast<char*>(args[i]);
        argv[args.size()] = nullptr;

        const int fd = open(_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd == VB_NVALUE) {
            _Exit(127);
        }

        if(dup2(fd, STDOUT_FILENO) == VB_NVALUE) {
            close(fd);
            _Exit(127);
        }
        if(dup2(fd, STDERR_FILENO) == VB_NVALUE) {
            close(fd);
            _Exit(127);
        }
        close(fd);

        const std::array<const char*, 2> export_path = {path.c_str(), nullptr};
        if (execve(argv[0], argv.data(), const_cast<char* const*>(export_path.data())) == VB_NVALUE) {
            _Exit(127);
        }
    }
    return 0;
}



std::string neosys::process::readFile(const std::string &path, char separator) {
    std::ifstream reader(path, std::ios::binary);
    if (!reader.is_open())
        return {};

    // Chunked read: works for regular files and for size-lying pseudo-files
    // (/proc), and does not trip the istreambuf_iterator null-dereference
    // false positive that libstdc++ emits at -O3.
    std::string body;
    std::array<char, 16384> chunk{};
    while (reader) {
        reader.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        body.append(chunk.data(), static_cast<size_t>(reader.gcount()));
    }

    if (separator != '\0')
        std::replace(body.begin(), body.end(), '\n', separator);

    return body;
}

int neosys::process::writeFile(const std::string &path, const std::string &content) {
    std::ofstream write_stream(path, std::ios::binary);
    if (!write_stream.is_open())
        return VB_NVALUE;

    write_stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    write_stream.close();
    return write_stream.good() ? VB_OK : VB_NVALUE;
}
