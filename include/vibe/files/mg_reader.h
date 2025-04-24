#ifndef MG_READERS_HPP
#define MG_READERS_HPP

#include <memory>
#include <string>
#include <filesystem>
#include <utility>
#include "../sysop/literals.h"


using std::make_shared;
using std::shared_ptr;
using std::string;

class MgReader
{
public:
    MgReader() = default;
    ~MgReader() = default;

    static std::pair<string,string> processing(const string &path, const int reserve)
    {

        if(!std::filesystem::exists(std::filesystem::path(path))) {
            return {literals::noPath(path), "404"};
        }
            try {
                size_t init = 0;
                string folder_base;

                for (int iterator = path.length(); iterator >= 0; iterator--)
                {
                    if (path[iterator] == '/')
                    {
                        init = iterator;
                        break;
                    }
                }
                for (size_t it = 0x0; it <= init; it++)
                {
                    folder_base += path[it];
                }

                if (!std::filesystem::exists(folder_base))
                {
                    return {notify_html::noPath(), "404"};
                }

                string body = process::readFile(path);
                string buffer{};

                for (int global = 0x0; global < reserve; global++)
                {
                    buffer = tratament(body, folder_base);
                    body = buffer;
                }

                return {body, "404"};
            } catch (std::exception &e) {
                return {e.what(), "500"};
            }
    }

     static string tratament(string &body, const string &folder_base)
    {

        std::pair<int, int> coord;
        int safe = 0;
        for (size_t it = 0; it < body.length(); it++)
        {

            string eye;
            eye += body[it];
            eye += body[it + 1];

            if (eye == OPEN)
            {
                body[it] = 0x20;
                body[it + 1] = 0x20;
                coord.first = it + 0x2;
                safe = 1;
            }
            else if (eye == CLOSE)
            {
                safe = 2;
                body[it] = 0x20;
                body[it + 1] = 0x20;
                coord.second = it - 1;
                break;
            }
        }

        if (safe == 1)
            return notify_html::noSafe();

        string name;

        for (int it = coord.first; it <= coord.second; it++)
        {
            name += body[it];
            body[it] = 0x20;
        }

        const string target = folder_base + name;
        if (!std::filesystem::exists(target))
        {
            return notify_html::noFIle(name);
        }

        body.insert(coord.first, process::readFile(target));
        string newest = body;
        return newest;
    }

    static  string normalize(string target)
    {
        string temp_box;
        for (char & i : target){
            if (i == 32)
                i = '\0';
            temp_box += i;
        }
        return temp_box;
    }
};

#endif // ! MG_READERS_HPP