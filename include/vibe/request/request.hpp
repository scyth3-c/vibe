#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include "parameters.hpp"
#include "../sysop/sysprocess.h"

using std::string;
using std::vector;

constexpr const char* NOT_PARAMS = "___s";

class Request {
  string route{};

    vector<std::pair<string, string>> _parameters{};
    vector<std::pair<string, string>> _headers{};

public:
  Request();

  [[nodiscard]] inline size_t total_parameters() const { return _parameters.size(); }
  [[nodiscard]] inline size_t total_headers() const {return _headers.size();}
  [[nodiscard]] Param_t getParameters();
  [[nodiscard]] Param_t getHeaders();

   inline void clear_parameters() noexcept {
       _parameters.clear();
       route.clear();
   };

  void setRawParametersData(string &&);
  void setRawHeadersData(string &&);
};

#endif // ! REQUEST_HPP