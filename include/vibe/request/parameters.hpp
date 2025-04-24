#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <utility>
#include <vector>
#include <string>
#include  <sstream>
#include <chrono>

using std::string;
using std::vector;

class param_box{
     std::pair<string,string> _body;

    public:

    param_box(string, string);
    explicit param_box(std::pair<string,string> content) : _body(content) {
        name = std::move(content.first);
        value = std::move(content.second);
 }  

    string name;
    string value;
    
};


class Param_t {
  vector<std::pair<string,string>> _list;
  
public: 

  Param_t();
  ~Param_t(){_list.clear();}

  [[maybe_unused]] explicit Param_t(vector<std::pair<string,string>> list) :  _list(std::move(list)) {}

  param_box operator[](int) const;

  void setContent(vector<std::pair<string,string>>&);

  [[maybe_unused]] inline void clear() { _list.clear(); }
  [[maybe_unused]] [[nodiscard]] inline bool empty() const {return _list.empty();}

  [[maybe_unused]] bool exist(const string&);

  [[maybe_unused]] param_box get(const string&);
  [[nodiscard]] inline vector<std::pair<string,string>> toArray() const noexcept {return _list;}

};


//
// struct utility_t {
//   static string prepare_basic(const string& _txt, const string& _type, const string& headers, const string& status="200") {
//
//     std::stringstream body;
//     body <<  "HTTP/1.1 " << status << " OK\n"
//          <<  "Server: Vibe/1.0\n"
//          <<  "Content-Type: "<<_type<<"\n"
//          <<  "Cache-Control: Expires"
//          <<  "Content-Length: " << std::to_string(_txt.length()) << "\n"
//          <<  "Accept-Ranges: bytes\n"
//          <<  headers
//          <<  "Connection: close\n"
//          <<  "\n"
//          <<  _txt;
//     return body.str();
//   }
//
//     [[maybe_unused]] static string guard_route(const std::chrono::duration<double>::rep seconds, string msg = "") {
//
//     std::stringstream message;
//     message << R"lit({"message":")lit"<< string(not msg.empty() ? std::move(msg) : "wait, this route has a " + std::to_string(static_cast<int>(seconds))+" second cooldown") << R"lit("})lit";
//
//     return prepare_basic( message.str(), "application/json", "", "401");
//   }
//
//     [[maybe_unused]] static int toInt(const string& data) {
//     try {
//         std::size_t pos;
//       const int result = std::stoi(data, &pos);
//       if(pos != data.size())
//         throw std::out_of_range(data);
//       return result;
//     } catch (const std::exception &e) {
//       return  0;
//     }
//   }
//
// };




#endif // PARAMETERS_HPP