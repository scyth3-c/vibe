#include "../../include/vibe/request/parameters.hpp"

#include <algorithm>
#include <utility>

param_box::param_box(string _name, string _value){
      name = std::move(_name);
      value = std::move(_value);
}

Param_t::Param_t(){
    _list.emplace_back("NULL","NULL");
}
param_box Param_t:: operator[](const int index) const
{
       param_box nuevo(_list[index]);
       return nuevo;
}

void Param_t::setContent(vector<std::pair<string,string>> &list) {
    _list.clear();
    _list = list;
}


[[maybe_unused]] bool Param_t::exist(const string& param){
    return std::any_of(_list.begin(), _list.end(), [&](const std::pair<string,string> &list) -> bool {
        return list.first == param;
    });
}


param_box Param_t::get(const string& param){
    const auto item = std::find_if(_list.begin(), _list.end(), [&](const std::pair<string,string> &iter) {
           return (iter.first == param);
       });

    if(item != _list.end()) {
        return {item->first, item->second};
    }
    return {"null", "null"};
}