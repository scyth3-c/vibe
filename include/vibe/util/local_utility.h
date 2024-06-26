#ifndef UTILITY_HPP
#define UTILITY_HPP

constexpr const char* BASE = "#include <iostream> \n int main() { \n ";

constexpr const char* WORK_PATH = "./";
constexpr char CODE_LOCATE = '$'; // ReadFileX() c++ code segment

constexpr const char* GPP_  = "g++ ";
constexpr const char* CPP_  = ".cpp";
constexpr const char* TXX_  = ".txt";
constexpr const char* OUT   = " -o ";
constexpr const char* AFTER = " && ./";
constexpr const char* DATA  = " > ";
constexpr const char* CODE_END = "\n return 0;\n}";

constexpr const char* OPEN = "#[";
constexpr const char* CLOSE = "];";

constexpr const char* OPEN_DATA = "[[";
constexpr const char* CLOSE_DATA = "]]";


#endif // ! UTILITY_HPP