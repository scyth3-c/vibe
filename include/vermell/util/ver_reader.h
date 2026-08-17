#ifndef VER_READERS_HPP
#define VER_READERS_HPP

#include <filesystem>
#include <string>
#include <utility>

#include "notify.h"
#include "local_utility.h"
#include "render_security.h"
#include "secure_render.h"

using std::string;

class VerReader
{
    // Upper bound for the include-expansion passes requested via compose():
    // each pass resolves one "#[module];" block.
    static constexpr int MAX_PASSES = 64;

    enum class Tag { Done, Ok, Unclosed, BadName, NotFound, TooLarge, IoError };

public:
    VerReader() = default;
    ~VerReader() = default;

    static std::pair<string, string> processing(const string &path, const int reserve,
                                                const vermell::RenderSecurity& sec = {})
    {
        if (!vermell::srender::is_within(vermell::effective_root(sec), path))
            return {notify::noPath(path), "403"};

        try {
            auto read = vermell::srender::read_bounded(path, sec.max_file_bytes);
            if (read.err != vermell::srender::ReadErr::Ok)
                return {notify::noPath(path), vermell::srender::status_of(read.err)};

            string body = std::move(read.data);

            // Includes live next to the template; an empty parent means CWD.
            std::filesystem::path folder = std::filesystem::path(path).parent_path();
            const string folder_base = folder.empty() ? "." : folder.string();

            const int passes = reserve < 0 ? 0 : (reserve > MAX_PASSES ? MAX_PASSES : reserve);
            for (int global = 0; global < passes; global++)
            {
                auto [tag, out] = tratament(body, folder_base, sec);
                switch (tag) {
                    case Tag::Done:
                        return {body, "200"};
                    case Tag::Ok:
                        body = std::move(out);
                        // A module that (transitively) includes itself would
                        // otherwise grow the page by up to max_file_bytes per
                        // pass, MAX_PASSES times: a memory-exhaustion DoS.
                        // Cap the composed result at the same limit the
                        // readers enforce for a single file.
                        if (body.size() > sec.max_file_bytes)
                            return {"Vermell: the composed page exceeds the allowed size", "413"};
                        continue;
                    case Tag::Unclosed:
                        return {notify_html::noSafe(), "400"};
                    case Tag::BadName:
                        return {notify_html::badName(out), "403"};
                    case Tag::TooLarge:
                        return {"Vermell: a module exceeds the allowed size", "413"};
                    case Tag::NotFound:
                        return {notify_html::noFIle(out), "404"};
                    default:
                        return {"Vermell: internal error while composing", "500"};
                }
            }

            return {body, "200"};
        } catch (std::exception &e) {
            std::cerr << "vermell compose internal error: " << e.what() << std::endl;
            return {"Vermell: internal error while composing", "500"};
        }
    }

private:
     // Expands the first "#[name];" block found in `body`.
     // Tag::Done leaves `body` untouched (no block present).
     static std::pair<Tag, string> tratament(string &body, const string &folder_base,
                                             const vermell::RenderSecurity& sec)
    {
        size_t open = string::npos;
        size_t close = string::npos;

        for (size_t it = 0; it + 1 < body.length(); it++)
        {
            if (body[it] == OPEN[0] && body[it + 1] == OPEN[1])
                open = it;
            if (body[it] == CLOSE[0] && body[it + 1] == CLOSE[1]) {
                close = it;
                break;
            }
        }

        if (open == string::npos && close == string::npos)
            return {Tag::Done, {}};
        if (open == string::npos || close == string::npos || close < open)
            return {Tag::Unclosed, {}};

        const string name = body.substr(open + 2, close - open - 2);

        // Bare file names only: this rejects "../", absolute paths, NULs and
        // any shell-ish trick before the path is ever built.
        if (!vermell::srender::valid_include_name(name))
            return {Tag::BadName, name};

        const string target = folder_base + "/" + name;

        // Defense in depth: the resolved module must stay inside the
        // template folder even if a symlink points outside.
        if (!vermell::srender::is_within(folder_base, target))
            return {Tag::BadName, name};

        auto read = vermell::srender::read_bounded(target, sec.max_file_bytes);
        switch (read.err) {
            case vermell::srender::ReadErr::Ok:
                break;
            case vermell::srender::ReadErr::NotFound:
                return {Tag::NotFound, name};
            case vermell::srender::ReadErr::TooLarge:
                return {Tag::TooLarge, name};
            default:
                return {Tag::NotFound, name};
        }

        return {Tag::Ok, body.substr(0, open) + read.data + body.substr(close + 2)};
    }
};

#endif // ! VER_READERS_HPP
