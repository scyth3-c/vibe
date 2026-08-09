#include "../../include/vibe/routes.hpp"


string  Query::getData() const noexcept                   {     return last;     }
bool    Query::getNext() const noexcept                   {     return next_enable;   }
void    Query::next()    noexcept                         {     next_enable = true;   }
void    Query::lock()    noexcept                         {     next_enable = false;  }


void Query::json(const string& _txt, const std::function<void()>& callback) noexcept {
    last = utility_t::prepare_basic(_txt, "application/json", headers);
    callback();
}
void Query::html(const string& _txt,  const std::function<void()>& callback) noexcept {
    last = utility_t::prepare_basic(_txt, "text/html", headers);
    callback();
}
void  Query::send(const string& _txt,  const std::function<void()>& callback) noexcept {
     last = utility_t::prepare_basic(_txt, "text/plain", headers);
     callback();
}
void Query::json(const string& _txt, const int status, const std::function<void()>& callback) noexcept {
    last = utility_t::prepare_basic(_txt, "application/json", headers, std::to_string(status));
    callback();
}
void Query::html(const string& _txt, const int status,  const std::function<void()>& callback) noexcept {
    last = utility_t::prepare_basic(_txt, "text/html", headers,  std::to_string(status));
    callback();
}
void  Query::send(const string& _txt,const int status,  const std::function<void()>& callback) noexcept {
     last = utility_t::prepare_basic(_txt, "text/plain", headers,  std::to_string(status));
     callback();
}

void  Query::readFile(const string& path,const string& type, const std::function<void()>& callback) noexcept {
    auto [data, status] =BasicRead::processing(path);
    last = utility_t::prepare_basic(data, type, headers, status);
    callback();
}
void  Query::readFile(const string& path, const std::function<void()>& callback) noexcept {
    auto [data, status] = BasicRead::processing(path);
    last = utility_t::prepare_basic(data, string(vibe::mime::of(path)), headers, status);
    callback();
}
void  Query::file(const string& path, const std::function<void()>& callback) noexcept {
    auto [data, status] = BasicRead::processing(path);
    last = utility_t::prepare_basic(data, string(vibe::mime::of(path)), headers, status);
    callback();
}
void  Query::readFileX(const string& path,const string& type, const std::function<void()>& callback) noexcept {
    auto [data, status] = CppReader::processing(path);
    last = utility_t::prepare_basic(data, type, headers, status);
    callback();
}

void  Query::compose(const string& path, const int reserve, const std::function<void()>& callback) noexcept {
    auto[data, status] = MgReader::processing(path, reserve);
    last = utility_t::prepare_basic(data, "text/html" , headers, status);
    callback();
}

void  Query::render(const string& path, const std::function<dataRender(dataRender& data)>& callback) noexcept {

 auto tasty_temp = std::make_unique<dataRender>(callback);
last = utility_t::prepare_basic(tasty_temp->render(path), "text/html", headers);
tasty_temp.reset();
}

void Query::setHeaders(const string& _body) noexcept {
 headers += _body + "\n";
}

void Query::setHeaders(HEADERS box) noexcept{
headers += box.generate();
}

long Query::getTimeKey() const noexcept {
    return time_key;
}
void Query::guard(const long &key, string custom_msg) noexcept {
    if(not custom_msg.empty())
      guardMsg = std::move(custom_msg);

    time_key = key;
}

string Query::getGuardMsg() const noexcept {
    return guardMsg;
}


