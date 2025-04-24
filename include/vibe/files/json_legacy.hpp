#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <vector>
#include <initializer_list>

using std::string;
using std::vector;
using std::initializer_list;

template<class...P>
struct JSON_t {
   [[maybe_unused]] JSON_t(initializer_list<P...> list) : body(std::move(list))  {}
    vector<string> body;
    [[nodiscard]] string json() const {
        string response{"{ "};

        constexpr char comilla = 34;
        constexpr char puntos = 58;
        constexpr char coma_simple =44;

        for (size_t i = 0; i < body.size(); i+=2) {
            response +=  comilla + body[i] + comilla;
            response +=  puntos;
            response +=  comilla + body[i+1] + comilla;
            response +=  coma_simple;
        }
        const size_t size = response.length();
        response[size-1] = 32;
        response += " }";

        for (size_t i = 0; i < response.length(); i++)
        {
            if(response[i] == '{' && i>0) {
                response[i-1] = ' ';
            } else if( response[i] == '}'){
                response[i+1] = ' ';
            }
        }
    
        return response;
    }

    string operator()() const{
        return json();
    }
};
using JSON_s = JSON_t<string>;

#endif // ! JSON_HPP