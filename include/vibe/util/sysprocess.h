//
// Created by scythe on 5/07/23.
//
// procesos del sistema

#ifndef VIBE_SYSPROCESS_H
#define VIBE_SYSPROCESS_H

#include <chrono>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
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

    class process {
       static  const char* const log_path;
       static  const std::string path;
    public:
        [[maybe_unused]] static inline void _wait(int milliseconds)  { std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); }
        [[maybe_unused]] static int run_command(const std::vector<const char*> &args, const std::string& _path = path);
        [[maybe_unused]] static std::mt19937& get_rng();
        [[maybe_unused]] static unsigned long random();
        [[maybe_unused]] static std::string readFile(const std::string &path);
        [[maybe_unused]] static int writeFile(const std::string &path, const std::string &content);

        /* system process here */
    };

}

#endif //VIBE_SYSPROCESS_H
