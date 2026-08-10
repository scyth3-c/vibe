#ifndef MG_READERS_HPP
#define MG_READERS_HPP

#include <filesystem>
#include <string>
#include <utility>

#include "notify.h"
#include "local_utility.h"
#include "render_security.h"
#include "secure_render.h"

using std::string;

class MgReader
{
    // Upper bound for the include-expansion passes requested via compose():
    // each pass resolves one "#[module];" block.
    static constexpr int MAX_PASSES = 64;

    enum class Tag { Done, Ok, Unclosed, BadName, NotFound, TooLarge, IoError };

public:
    MgReader() = default;
    ~MgReader() = default;

    static std::pair<string, string> processing(const string &path, const int reserve,
                                                const vibe::RenderSecurity& sec = {})
    {
        if (!sec.root.empty() && !vibe::srender::is_within(sec.root, path))
            return {notify::noPath(path), "403"};

        try {
            auto read = vibe::srender::read_bounded(path, sec.max_file_bytes);
            if (read.err != vibe::srender::ReadErr::Ok)
                return {notify::noPath(path), vibe::srender::status_of(read.err)};

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
                        continue;
                    case Tag::Unclosed:
                        return {notify_html::noSafe(), "400"};
                    case Tag::BadName:
                        return {notify_html::badName(out), "403"};
                    case Tag::TooLarge:
                        return {"Vibe: a module exceeds the allowed size", "413"};
                    case Tag::NotFound:
                        return {notify_html::noFIle(out), "404"};
                    default:
                        return {"Vibe: internal error while composing", "500"};
                }
            }

            return {body, "200"};
        } catch (std::exception &e) {
            std::cerr << "vibe compose internal error: " << e.what() << std::endl;
            return {"Vibe: internal error while composing", "500"};
        }
    }

private:
     // Expands the first "#[name];" block found in `body`.
     // Tag::Done leaves `body` untouched (no block present).
     static std::pair<Tag, string> tratament(string &body, const string &folder_base,
                                             const vibe::RenderSecurity& sec)
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
        if (!vibe::srender::valid_include_name(name))
            return {Tag::BadName, name};

        const string target = folder_base + "/" + name;

        // Defense in depth: the resolved module must stay inside the
        // template folder even if a symlink points outside.
        if (!vibe::srender::is_within(folder_base, target))
            return {Tag::BadName, name};

        auto read = vibe::srender::read_bounded(target, sec.max_file_bytes);
        switch (read.err) {
            case vibe::srender::ReadErr::Ok:
                break;
            case vibe::srender::ReadErr::NotFound:
                return {Tag::NotFound, name};
            case vibe::srender::ReadErr::TooLarge:
                return {Tag::TooLarge, name};
            default:
                return {Tag::NotFound, name};
        }

        return {Tag::Ok, body.substr(0, open) + read.data + body.substr(close + 2)};
    }
};

#endif // ! MG_READERS_HPP
