#ifndef CPP_READER_HPP
#define CPP_READER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "local_utility.h"
#include "nterminal.h"
#include "sysprocess.h"

using std::string;
using std::make_shared;
using std::shared_ptr;
using neosys::process;

class CppReader {
public:
    CppReader() = default;
     static std::pair<string,string> processing(const string& path) {

        if(!std::filesystem::exists(std::filesystem::path(path)))
            return {notify::noPath(path), "404"};

        try {

        bool init = false;
        std::pair<int, int> coords;
        string code = BASE;
        string raw_html = process::readFile(path);

        for (size_t iterator = 0; iterator < raw_html.length() - 1; iterator++) {
            if (init) {
                if (raw_html[iterator] == CODE_LOCATE) {
                    coords.second = static_cast<int>(iterator);
                    break;
                }
                    code += raw_html[iterator];
            }
            if (raw_html[iterator] == CODE_LOCATE && !init){
                coords.first = static_cast<int>(iterator);
                init = true;
            }
        }


            std::string file_path, code_file, file_result;

            file_path.append(WORK_PATH).append(std::to_string(process::random()));

            code_file.append(file_path).append(CPP_);
            file_result.append(file_path).append(TXX_);

            code.append(CODE_END);
            process::writeFile(code_file, code);

            vector<const char*> compile;
            compile.push_back("/bin/g++");
            compile.push_back("-std=c++17");
            compile.push_back( code_file.c_str());
            compile.push_back( "-o" );
            compile.push_back( file_path.c_str());
            compile.push_back( nullptr);

            if(process::run_command(compile, file_result) == VB_NVALUE) {
                std::filesystem::remove(code_file);
                return { replaceCoords(raw_html, coords, readAndRemove(file_result)) ,"400"};
            }

            vector<const char*> execute;
            execute.push_back( file_path.c_str());
            execute.push_back( nullptr);

        if(process::run_command(execute, file_result) == VB_NVALUE) {
                std::filesystem::remove(file_path);
                std::filesystem::remove(code_file);
                return { replaceCoords(raw_html, coords, readAndRemove(file_result)) ,"400"};
            }

        const string execution_process = process::readFile(file_result);

        std::filesystem::remove(file_path);
        std::filesystem::remove(code_file);
        std::filesystem::remove(file_result);

        return { replaceCoords(raw_html, coords, execution_process) ,"200"};
        }catch (std::exception &e) {
            return {e.what(), "500"};
        }
    }

    static string replaceCoords(string content, const std::pair<int, int > &coords, const string& data) {
         for (int i = coords.first; i <= coords.second; i++)
             content[i] = 32;
         content.insert(coords.first, data);
         return  content;
     }
    static string readAndRemove(const string& path) {
        string temp = process::readFile(path);
        std::filesystem::remove(path);
         return temp;
     }
};

#endif // ! CPP_READER_HPP