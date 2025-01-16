
#ifndef BASIC_READER_HPP
#define BASIC_READER_HPP

#include <string>
#include <fstream>
#include <filesystem>

#include "notify.h"

using std::string;

class BasicRead {
public:
    BasicRead() = default;

    static std::pair<string, string> processing(const string& path) {
        if(!std::filesystem::exists(std::filesystem::path(path))) {
            return {notify::noPath(path), "404"};
        }
        try {
            return {neosys::process::readFile(path), "200"};
        } catch (std::exception &e) {
            return {e.what(), "500"};
        }
    }
};

#endif // !BASIC_READER_HPP