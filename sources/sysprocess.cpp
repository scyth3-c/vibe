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

int neosys::process::run_command(const std::vector<const char*> &args, const std::string& _path) {
    int status;

    if (args.empty() || !static_cast<bool>(args[0])) {
        return VB_NVALUE;
    }

    if (const pid_t pid = fork(); pid == VB_NVALUE) {
        return VB_NVALUE;
    } else if (pid != 0) {
        pid_t retval;
        while ((retval = waitpid(pid, &status, 0)) != VB_NVALUE) {
            if (retval == pid) break;
            if (errno == EINTR) continue;
        }
        if (retval == VB_NVALUE || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return VB_NVALUE;
        }
    } else {

        std::vector<char*> mutable_args;
        mutable_args.reserve(args.size());
        for (const char* arg : args) {
            mutable_args.push_back(const_cast<char*>(arg));
        }
        mutable_args.push_back(nullptr);

        const int fd = open(_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd == VB_NVALUE) {
            std::cerr << strerror(errno) << std::endl;
            _Exit(127);
        }

        if(dup2(fd, STDOUT_FILENO) == VB_NVALUE) {
            std::cerr << strerror(errno) << std::endl;
            close(fd);
            exit(127);
        }
        if(dup2(fd, STDERR_FILENO) == VB_NVALUE) {
            std::cerr << strerror(errno) << std::endl;
            close(fd);
            exit(127);
        }
        close(fd);

        const std::array<const char*, 2> export_path = {path.c_str(), nullptr};
        if (execve(mutable_args[0], mutable_args.data(),  const_cast<char* const*>(export_path.data())) == VB_NVALUE) {
            std::cerr << strerror(errno) << std::endl;
            _Exit(127);
        }
    }
    return 0;
}



std::string neosys::process::readFile(const std::string &path, char separator) {
    std::ifstream reader(path, std::ios::binary);
    if (!reader.is_open())
        return {};

    std::string body{std::istreambuf_iterator<char>(reader), std::istreambuf_iterator<char>()};

    if (separator != '\0')
        std::replace(body.begin(), body.end(), '\n', separator);

    return body;
}

int neosys::process::writeFile(const std::string &path, const std::string &content) {
    try {
        std::ofstream write_stream;
        write_stream.open(path.c_str());

        for (const auto &it : content)
            write_stream << it;

        write_stream.close();
        return VB_OK;
    }catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return VB_NVALUE;
    }
}
