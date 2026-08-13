//
// Created by scythe on 5/07/23.
//
// procesos del sistema

#ifndef VERMELL_SYSPROCESS_H
#define VERMELL_SYSPROCESS_H

#include <chrono>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <string>
#include <fcntl.h>
#include <random>
#include <fstream>
#include <array>

namespace neosys {

    // Sandboxing knobs for run_command(). All limits are applied in the
    // child right before execve(); the defaults reproduce the legacy
    // behavior (no limits, wait forever).
    struct RunOptions {
        // Wall-clock budget. 0 = wait forever (legacy). On expiration the
        // whole process group is SIGKILLed and VER_NVALUE is returned.
        std::chrono::milliseconds timeout{0};
        rlim_t cpu_seconds     = 0; // RLIMIT_CPU   (0 = unlimited)
        rlim_t memory_bytes    = 0; // RLIMIT_AS    (0 = unlimited)
        rlim_t file_size_bytes = 0; // RLIMIT_FSIZE (0 = unlimited)
        rlim_t max_processes   = 0; // RLIMIT_NPROC (0 = unlimited)
        // chdir() here inside the child: an empty private directory denies
        // the program any view of the server's working directory.
        const char* work_dir = nullptr;
        // setsid() in the child so the timeout can kill the whole group.
        bool new_session = false;
    };

    class process {
       static  const char* const log_path;
       static  const std::string path;
    public:
        [[maybe_unused]] static inline void _wait(int milliseconds)  { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }
        [[maybe_unused]] static int run_command(const std::vector<const char*> &args,
                                                const std::string& _path = path,
                                                const RunOptions& opts = {});
        [[maybe_unused]] static std::mt19937& get_rng();
        [[maybe_unused]] static unsigned long random();
        [[maybe_unused]] static std::string readFile(const std::string &path, char separator = '\0');
        [[maybe_unused]] static int writeFile(const std::string &path, const std::string &content);

        /* system process here */
    };

}

#endif //VERMELL_SYSPROCESS_H
