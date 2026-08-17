
#ifndef BASIC_READER_HPP
#define BASIC_READER_HPP

#include <iostream>
#include <string>
#include <utility>

#include "notify.h"
#include "render_security.h"
#include "secure_render.h"

using std::string;

class BasicRead {
public:
    BasicRead() = default;

    // Serves a file hardened against the usual abuses:
    //   - optional jail (sec.root): canonical containment, symlinks resolved
    //   - regular files only (no FIFOs, devices, /proc lies)
    //   - bounded size (sec.max_file_bytes)
    //   - generic client errors; diagnostics stay on the server's stderr
    static std::pair<string, string> processing(const string& path,
                                                const vermell::RenderSecurity& sec = {}) {
        if (!vermell::srender::is_within(vermell::effective_root(sec), path))
            return {notify::noPath(path), "403"};

        auto read = vermell::srender::read_bounded(path, sec.max_file_bytes);
        switch (read.err) {
            case vermell::srender::ReadErr::Ok:
                return {std::move(read.data), "200"};
            case vermell::srender::ReadErr::NotFound:
                return {notify::noPath(path), "404"};
            case vermell::srender::ReadErr::TooLarge:
                return {"Vermell: the requested file exceeds the allowed size", "413"};
            case vermell::srender::ReadErr::Forbidden:
                return {notify::noPath(path), "403"};
            default:
                return {"Vermell: internal error while reading the file", "500"};
        }
    }
};

#endif // !BASIC_READER_HPP
