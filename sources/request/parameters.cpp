#include "../../include/vermell/request/parameters.hpp"

#include <algorithm>

param_box::param_box(string _name, string _value){
      name = std::move(_name);
      value = std::move(_value);
}

param_box Param_t::operator[](const int index) {
    if (index < 0 || static_cast<size_t>(index) >= _list.size())
        return param_box{"null", "null"};
    return param_box(_list[static_cast<size_t>(index)]);
}

void Param_t::setContent(const vector<std::pair<string, string>> &list) {
    _list = list;
}


[[maybe_unused]] bool Param_t::exist(const string& param){
    return exist(std::string_view(param));
}

bool Param_t::exist(const std::string_view name) const {
    return std::any_of(_list.begin(), _list.end(),
                       [&](const auto& item) { return item.first == name; });
}


param_box Param_t::get(const string& param){
    const auto item = std::find_if(_list.begin(), _list.end(),
                                   [&](const auto& iter) { return iter.first == param; });

    if (item != _list.end())
        return param_box{item->first, item->second};

    return param_box{"null", "null"};
}

string Param_t::value_or(const std::string_view name, string fallback) const {
    const auto item = std::find_if(_list.begin(), _list.end(),
                                   [&](const auto& iter) { return iter.first == name; });
    return item != _list.end() ? item->second : std::move(fallback);
}
