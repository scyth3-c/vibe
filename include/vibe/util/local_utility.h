#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
using std::string;

constexpr auto BASE = "#include <iostream> \n int main() { \n ";

constexpr auto WORK_PATH = "./";
constexpr char CODE_LOCATE = '$'; // ReadFileX() c++ code segment

constexpr auto CPP_  = ".cpp";
constexpr auto TXX_  = ".txt";

constexpr auto CODE_END = "\n return 0;\n}";

constexpr auto OPEN = "#[";
constexpr auto CLOSE = "];";

constexpr auto OPEN_DATA = "[[";
constexpr auto CLOSE_DATA = "]]";


#endif // ! UTILITY_HPP