#include "../../include/vibe/request/request.hpp"

Request::Request()= default;


void Request::setRawParametersData(string &&_raw) {

  if (_raw == NOT_PARAMS)
    return;

  const char EQUAL = '=';
  const char ASP = '&';

  _raw.at(0) = _raw.at(0) == '\n' ? '\0' : _raw.at(0);
  _raw.push_back(ASP);

  size_t midd = 0, end = 0;
  while (!_raw.empty()) {

    midd = _raw.find(EQUAL);
    end = _raw.find(ASP);

    if(midd == string::npos || end == string::npos)
      break;

    string chunk = _raw.substr(0,end),
    first = chunk.substr(0, midd), name;

    std::copy_if(first.begin(), first.end(), std::back_inserter(name), [](unsigned char c) { return std::isprint(c); });

    _parameters.emplace_back(name, chunk.substr(midd+1, end));
    _raw.erase(0, end+1);
  }

}

void Request::setRawHeadersData(string &&_raw){

  const char COLON = ':';
  const char CR = '\r';

  size_t midd = 0, end = 0;

  while (!_raw.empty()) {
    midd = _raw.find(COLON);
    end = _raw.find(CR);

    if(midd == string::npos || end == string::npos)
      break;
    if(midd == 0 || midd + 2 > end)
      break;

    string chunk = _raw.substr(0,end);
    _headers.emplace_back(chunk.substr(1, midd-1), chunk.substr(midd+2, end));
    _raw = _raw.erase(0, end+1);
  }
}

Param_t Request::getHeaders() {
  Param_t nuevo;
  try {
    nuevo.setContent(_headers);
    return nuevo;
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
  return nuevo;
}

Param_t Request::getParameters() {
    Param_t nuevo;
    try {
       nuevo.setContent(_parameters);
       return nuevo;
    }
    catch(const std::exception& e) {
      std::cerr << e.what() << '\n';
    }
    return nuevo;
}