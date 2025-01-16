#include "../include/vibe/util/parameter_proccess.h"

HTTP_QUERY::HTTP_QUERY() = default;
HTTP_QUERY::~HTTP_QUERY() = default;

string HTTP_QUERY::selectPerType(const string &target, const string &conten_type, bool &init) {
    if (target.empty())
        init = false;
    return conten_type == X_WWW_FORM ? x_www_form_urlencoded(target) :  raw_form_encoded(target);
}

string HTTP_QUERY::route_refactor_params(const string& _target)  {

    const string content_type = findContenType(_target);
    if(content_type != STR_ERR) {
        bool init = true;
        string encoded = selectPerType(_target, content_type , init);
        return init ? encoded : NOT_PARAMS;
    }
        return NOT_PARAMS;
}

string HTTP_QUERY::route_refactor_params_get(const string& rawresponse) {
    return get_params(rawresponse);
}


string HTTP_QUERY::headers_from(const string& response)  {
    string encoded{};
    char fc = 0x0A;
    for(size_t i = 0; i<response.size(); i++) {
        if(response[i] == fc){
            if(response[i+1]==fc)
                break;
            fc = '\0';
            encoded.clear();
        }
            encoded.push_back(response[i]);
    }
    return encoded;
}

std::pair<string, string> HTTP_QUERY::route_refactor(const string& target){
    const size_t size = target.size();
    std::pair<string, string> route;
    bool init = false;

    for (const auto &it : target){
        if (it == 0x20 || it == '/')
            break;
        route.first.push_back(it);
    }
    for (size_t ui = 0; ui < size; ui++){
        if (init){
            if (target[ui] == '?' || target[ui] == 'H')
                break;
            if (target[ui] != ' '){
                route.second.push_back(target[ui]);
            }
        }
        else if (target[ui + 0x1] == '/'){
            init = true;
        }
    }
    if(route.second.back() == '/' && route.second.length() > 1)
        route.second.pop_back();
    return route;
}

string HTTP_QUERY::x_www_form_urlencoded(const string &target){
    string params{};
    for (size_t ti = target.length() - 0x1; ti > 0x0; ti--){
        if (target[ti] == ' ' || target[ti] == 13)
            break;
        params.push_back(target[ti]);
    }
    std::reverse(params.begin(), params.end());
    return params;
}

string HTTP_QUERY::raw_form_encoded(const string &target) {
    const auto spacepos = target.find("\n\r");
    string body = RAW_TARGET;
    if (spacepos == string::npos) {
        return RAW_ERROR;
    }
    string chunk = target.substr(spacepos + 3);
    if(!chunk.empty()) {
        body.append(chunk);
        return body;
    }
    return RAW_ERROR;
}


string HTTP_QUERY::findContenType(const string &text) {
    if (text.empty())  { return STR_ERR; }

    std::string target = "Content-Type: ";

    const size_t ctpos = text.find(target);
    const size_t splpos = text.find (10, ctpos+target.length());

    if (ctpos == string::npos || splpos == string::npos) {
        return STR_ERR;
    }
    size_t i = ctpos + target.length();
    target.clear();
    for (; i<=splpos; i++)  {
        target.push_back(text.at(i));
    }
    return trim(target);
}


string HTTP_QUERY::get_params(const string &target){
    const auto start = target.find(63);
    const auto end = target.find(32, start);
    if(start == string::npos)
        return NOT_PARAMS;
    string params{};
    for(size_t it=start+1; it<end; it++) {
        params.push_back(target.at(it));
    }
    if(params.empty())
        return  NOT_PARAMS;
    return params;
}

string HTTP_QUERY::trim(string target){
    target.erase(std::remove_if(target.begin(), target.end(), ::isspace), target.end());
    return target;
}