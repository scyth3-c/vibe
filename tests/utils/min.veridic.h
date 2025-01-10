
// Archivo: http.hpp

#pragma once

#ifndef HTTP_H
#define HTTP_H

#define CURL_STATICLIB
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <vector>


#define PUT_TYPE "PUT"
#define POST_t "POST"
#define DELETE_TYPE "DELETE"
#define DEFAULT "00"

using std::string;



template<typename A,class...LIST>
struct VHeaders_t {

    std::vector<LIST...> list;
    VHeaders_t(std::initializer_list<LIST...> arguments) : list(arguments) {
          if (!list.empty()){
              for(auto &it : list) {
                  aloc.push_back(it);
              }
              initial = true;
          }
    }
	bool initial{false};
	std::vector<A> aloc{};
};
using VHeaders = VHeaders_t<string,string>;



template <typename A, class... LIST>
struct FIELDS
{
    std::vector<LIST...> list;
   FIELDS(std::initializer_list<LIST...> arguments) : list(arguments) {}

    string http_params{};
    string transform()
    {
        for (size_t i = 0; i < list.size(); i++){
        	http_params += list[i] + "&";
        }
		http_params[http_params.length()-1] = static_cast<char>(00);
        return http_params;
    }
};

typedef FIELDS<string, string> 	POST;
typedef FIELDS<string, string> 	PUT;
typedef FIELDS<string, string> 	GET;
typedef FIELDS<string, string>  DELETE;


class HTTP
{
private:
	std::shared_ptr<CURL *> curl = nullptr;

	std::vector<string> http_VHeaders;
	std::vector<CURLcode> performs;
	unsigned int queryIndice = 0;
	string URL{};
	string response{};
	bool headerify{false};

public:
	explicit HTTP(string);
	HTTP() = default;
	~HTTP();
	static size_t callback(const void *, size_t, size_t, string *);

	int getSimple(const string& endpoint_= DEFAULT);
	int getBase(GET *, const string &endpoint = DEFAULT, bool headers = false, const ::VHeaders *base = nullptr);
    int get(GET& fields, const VHeaders & headers, const string& endpoint = DEFAULT);
    int getVHeaders(const VHeaders &headers, const string& endpoint = DEFAULT);

    int postSimple(const string &endpoint = DEFAULT);
    int postVHeaders(const VHeaders& headers, const string& end = DEFAULT);
	int post(POST &, const string& endpoint = DEFAULT, const string& type = DEFAULT);
	int post(POST *, const VHeaders &, const string& endpoint = DEFAULT, const string& type = DEFAULT);


	int put(PUT &, const string& _endpoint=DEFAULT);
	int put(PUT &, const VHeaders &, const string& _endpoint = DEFAULT);

	int Delete(DELETE &, const string& _endpoint = DEFAULT);
	int Delete(DELETE &, const VHeaders &, const string& _endpoint = DEFAULT);

	int custom(POST &_fields, const string& type, const string& _endpoint = DEFAULT);
	int custom(POST &_fields, const VHeaders &headers, const string &type, const string& _endpoint = DEFAULT);


	static curl_slist* makeVHeaders(const std::vector<string>&);
    void setUrl(const string &);
	int setVHeaders(const VHeaders&);

	[[nodiscard]] string Response() const;
	static string without(string, char);
	string genPerfomList();
};

#endif // !HTTP_H
// Archivo: veridic.hpp

#pragma once

#ifndef VERIDIC_HPP
#define VERIDIC_HPP

#include <memory.h>
#define PLAIN_TEXT "Content-Type: text/plain"

using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;

class Veridic {

private:
    shared_ptr<HTTP> http = nullptr;
    string URL{};

public:
    Veridic();
    explicit Veridic(string );
    bool setUrl(const string&);

    string get(const string& endpoint = DEFAULT);
    string get(const VHeaders &, const string& endpoint = DEFAULT);
    string get(GET &, const string &endpoint = DEFAULT);
    string get(GET &, const VHeaders &, const string& endpoint = DEFAULT);


    string post(const string& endpoint = DEFAULT);
    string post(const VHeaders &, const string& endpoint = DEFAULT);
    string post(POST &, const string& endpoint = DEFAULT, const string &type = DEFAULT);
    string post(POST &, const VHeaders &, const string& endpoint = DEFAULT, const string& type = DEFAULT);

    string put(PUT &, const string& endpoint = DEFAULT);
    string put(PUT &, const VHeaders &, const string& _endpoint = DEFAULT);

    string Delete(DELETE &, const string& endpoint = DEFAULT);
    string Delete(DELETE &, const VHeaders &, const string& endpoint = DEFAULT);

    string custom(POST &field, const string& type, const string& endpoint = DEFAULT);
    string custom(POST &fields, const VHeaders &headers, const string& type, const string& endpoint = DEFAULT);
};

#endif

#ifndef UTILS_HPP
#define UTILS_HPP

constexpr auto UTILS_ERROR = "-[critic error]-";
constexpr auto UTILS_WARNING = "-[warning]-";
constexpr auto UTILS_SUCCESS = 0;

constexpr auto SYSTEM_DECORATOR = "-[BAD SYSTEM RESPONSE]-";
constexpr auto CURL_DECORATOR = "-[BAD CURL RESPONSE]-";
constexpr auto BAD_RESULT = "-[BAD_USE]-";

constexpr auto VHTTP_ERROR = -1;
constexpr auto CURL_ERROR = -2;
constexpr auto HTTP_SUCCESS = 0;

constexpr auto  UTILS_USER_ERROR = 3;

template<class... C>
auto screen(std::ostream& out, C const &...content) -> void {
    ((out<<content), ...);
}


#endif // UTILS_HPP!
// Archivo: http.cpp


inline HTTP::HTTP(string url) : URL(std::move(url)) {
	curl = std::make_shared<CURL *>(curl_easy_init());
	if (!curl) {
		screen(std::cout, UTILS_ERROR, SYSTEM_DECORATOR);
	}
}

inline HTTP::~HTTP() {curl.reset();}

// callback function, this extract response of querys
inline size_t HTTP::callback(const void *buffer, size_t size, size_t nmemb, string *data) {
	const size_t n = size * nmemb;
	try {
		data->append(static_cast<const char *>(buffer), n);
	}
	catch (std::bad_alloc &e) {
		screen(std::clog, UTILS_ERROR, SYSTEM_DECORATOR, &e);
	}
	return n;
}



// base http get
inline int HTTP::getSimple(const std::string& endpoint_) {
	const string temp_url = endpoint_ != DEFAULT ? URL +endpoint_ : URL;

    curl_easy_setopt(*curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(*curl, CURLOPT_URL, temp_url.c_str());
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, HTTP::callback);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);

	CURLcode perfom = curl_easy_perform(*curl);
	performs.push_back(perfom);

	try {
		if (perfom != CURLE_OK)
			screen(std::clog, UTILS_WARNING, CURL_DECORATOR, perfom);
	}
	catch (std::exception &error) {
		screen(std::cout, UTILS_ERROR, &error);
	}
	curl_easy_cleanup(*curl);
	return HTTP_SUCCESS;
}

inline int HTTP::getBase(GET *fields, const std::string& endpoint, const bool headers, const VHeaders *base) {

    string temp_url = endpoint != DEFAULT ? URL +endpoint : URL;
    struct curl_slist *VHeadersList = nullptr;

	if(fields != nullptr) {
        temp_url += "?" + fields->transform();
        fields = nullptr;
	}

    curl_easy_setopt(*curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(*curl, CURLOPT_URL, temp_url.c_str());
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, HTTP::callback);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);

    if (headers) {
      VHeadersList = makeVHeaders(base->aloc);
      if (VHeadersList != nullptr){
          curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, VHeadersList );
      }
    }

	CURLcode perfom = curl_easy_perform(*curl);
	performs.push_back(perfom);

    if (VHeadersList != nullptr){
        curl_slist_free_all(VHeadersList);
        base = nullptr;
    }

	try {
		if (perfom != CURLE_OK)
			screen(std::clog, UTILS_WARNING, CURL_DECORATOR, perfom);
	}
	catch (std::exception &error){
        screen(std::cout, UTILS_ERROR, &error);
		return CURL_ERROR;
	}
	curl_easy_cleanup(*curl);
	return HTTP_SUCCESS;
}

inline int HTTP::get(GET &fields, const VHeaders &headers, const std::string& endpoint)  {
    return HTTP::getBase(&fields, endpoint, true, &headers);
}

inline int HTTP::getVHeaders(const VHeaders &headers, const string& endpoint) {
    return HTTP::getBase(nullptr, endpoint, true, &headers);
}

// base http post
inline int HTTP::post(POST &fields, const string& endpoint, const string& type) {

    string temp_url = endpoint != DEFAULT ? URL +endpoint : URL;

	auto postFields = fields.transform();

    curl_easy_setopt(*curl, CURLOPT_URL, temp_url.c_str());
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, HTTP::callback);
	curl_easy_setopt(*curl, CURLOPT_POSTFIELDS, postFields.c_str());
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);

	if (type != DEFAULT) {
		curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, type.c_str());
	} else{
        curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, (const char*)POST_t);
    }

	CURLcode perfom = curl_easy_perform(*curl);
	performs.push_back(perfom);

	try {
		if (perfom != CURLE_OK)
			screen(std::clog, UTILS_WARNING, CURL_DECORATOR, perfom);
	}
	catch (std::exception &error) {
        screen(std::cout, UTILS_ERROR, &error);
		return CURL_ERROR;
	}
	curl_easy_cleanup(*curl);
	return HTTP_SUCCESS;
}

inline int HTTP::post(POST *fields, const VHeaders &headers, const string& endpoint, const string& type) {

    string temp_url = endpoint != DEFAULT ? URL +endpoint : URL;
    struct curl_slist *VHeadersList = nullptr;

	curl_easy_setopt(*curl, CURLOPT_URL, temp_url.c_str());
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, HTTP::callback);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);

    if(fields != nullptr) {
	    const auto postFields = fields->transform();
        curl_easy_setopt(*curl, CURLOPT_POSTFIELDS, postFields.c_str());
        fields = nullptr;
    }

    if (type != DEFAULT){
        curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, type.c_str());
    }

	VHeadersList = makeVHeaders(headers.aloc);
    curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, VHeadersList );

	CURLcode perfom = curl_easy_perform(*curl);
	performs.push_back(perfom);

    curl_slist_free_all(VHeadersList);

	try {
		if (perfom != CURLE_OK)
			screen(std::clog, UTILS_WARNING, CURL_DECORATOR, perfom);
	}
	catch (std::exception &error) {
        screen(std::cout, UTILS_ERROR, &error);
		return CURL_ERROR;
	}
	curl_easy_cleanup(*curl);
	return HTTP_SUCCESS;
}

inline int HTTP::postVHeaders(const VHeaders &headers, const std::string& end) {
    return HTTP::post(nullptr, headers, end, "POST");
}

inline int HTTP::postSimple(const std::string &endpoint) {
    string temp_url = endpoint != DEFAULT ? URL +endpoint : URL;

    curl_easy_setopt(*curl, CURLOPT_URL, temp_url.c_str());
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, HTTP::callback);
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, (const char*)POST_t);

    CURLcode perfom = curl_easy_perform(*curl);
    performs.push_back(perfom);

    try {
        if (perfom != CURLE_OK)
            screen(std::clog, UTILS_WARNING, CURL_DECORATOR, perfom);
    }
    catch (std::exception &error) {
        screen(std::cout, UTILS_ERROR, &error);
        return CURL_ERROR;
    }
    curl_easy_cleanup(*curl);
    return HTTP_SUCCESS;
}

inline int HTTP::put(PUT &_fields, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(_fields, _endpoint, PUT_TYPE) : post(_fields, PUT_TYPE);
}

inline int HTTP::put(PUT &_fields, const VHeaders &headers, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(&_fields, headers, _endpoint, PUT_TYPE) : post(&_fields, headers, PUT_TYPE);
}

inline int HTTP::Delete(DELETE &_fields, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(_fields, _endpoint, DELETE_TYPE) : post(_fields, DELETE_TYPE);
}

inline int HTTP::Delete(DELETE &_fields, const VHeaders &headers, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(&_fields, headers, _endpoint, DELETE_TYPE) : post(&_fields, headers, DELETE_TYPE);
}

inline int HTTP::custom(POST &_fields, const string& type, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(_fields, _endpoint, type) : post(_fields, type);
}

inline int HTTP::custom(POST &_fields, const VHeaders &headers, const string &type, const string& _endpoint) {
    return (_endpoint != DEFAULT) ? post(&_fields, headers, _endpoint, type) : post(&_fields, headers, type);
}

inline int HTTP::setVHeaders(const VHeaders &headers)
{

	if (!headers.initial)
		return UTILS_USER_ERROR;
	http_VHeaders = headers.aloc;
	headerify = true;
	return UTILS_SUCCESS;
}

inline curl_slist *HTTP::makeVHeaders(const std::vector<string>& headers) {
        struct curl_slist *list = nullptr;

        if (headers.empty())
            return list;

        for (auto &it : headers) {
            list = curl_slist_append(list, it.c_str());
        }
        return list;
}

inline void HTTP::setUrl(const string &_url){
	URL = _url;
}

// extract response of the querys
inline string HTTP::Response() const {
	return response;
}

// remove separator send of the API
inline string HTTP::without(string target, const char key) {
	for (auto &it : target) {
		if (it == key)
			it = static_cast<char>(00);
	}
	return target;
}

// extract the list of result of querys
inline string HTTP::genPerfomList(){
	int map{};
	for (const auto &it : performs){
		map += it;
	}
	return std::to_string(map);
}
// Archivo: veridic.cpp


inline Veridic::Veridic(string tasty_url) : URL(std::move(tasty_url)) {}

inline bool Veridic::setUrl(const string& url) {
    if (!url.empty()) {
        URL = url;
        return true;
    }
        return false;
}

inline string Veridic::get(const string& endpoint)
{
    if (URL.empty())
        return BAD_RESULT;
    http = make_shared<HTTP>(URL);

    if (http->getSimple(endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::get(GET &fields, const string &endpoint)
{
    if (URL.empty())
        return BAD_RESULT;
    http = make_shared<HTTP>(URL);

    if (http->getBase(&fields, endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::get(GET &fields, const VHeaders& headers, const string& endpoint)
{
    if (URL.empty())
        return BAD_RESULT;
    http = make_shared<HTTP>(URL);

    if (http->get(fields, headers, endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::get(const VHeaders& headers, const string& endpoint)
{
    if (URL.empty())
        return BAD_RESULT;
    http = make_shared<HTTP>(URL);

    if (http->getVHeaders(headers, endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
    return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}


inline string Veridic::post(POST &fields, const string& endpoint, const string &type)
{

    if (URL.empty())
        return BAD_RESULT;
    http = make_shared<HTTP>(URL);

    if (http->post(fields, endpoint != DEFAULT ? endpoint : "", !type.empty() ? type : "" ) != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::post(POST &fields, const VHeaders &headers, const string& endpoint, const string& type)
{

    if (URL.empty())
        return BAD_RESULT;
    if (!headers.initial)
        return BAD_RESULT;

    http = make_shared<HTTP>(URL);

    if (http->post(&fields, headers, endpoint != DEFAULT ? endpoint : "", !type.empty() ? type : "" ) != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::post(const std::string& endpoint) {
    if (URL.empty())
        return BAD_RESULT;

    http = make_shared<HTTP>(URL);

    if (http->postSimple(endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::post(const VHeaders &headers, const std::string& endpoint) {
    if (URL.empty())
        return BAD_RESULT;
    if (!headers.initial)
        return BAD_RESULT;

    http = make_shared<HTTP>(URL);

    if (http->postVHeaders(headers, endpoint != DEFAULT ? endpoint : "") != HTTP_SUCCESS)
        return BAD_RESULT;

    auto response = http->Response();
    http.reset();
    return response;
}

inline string Veridic::put(PUT &_fields, const string& endpoint) {
 return endpoint != DEFAULT ? post(_fields, endpoint, PUT_TYPE) :post(_fields, PUT_TYPE) ;
}

inline string Veridic::put(PUT &_fields, const VHeaders &headers, const string& _endpoint) {
    return _endpoint != DEFAULT ? post(_fields,headers, _endpoint, PUT_TYPE) :post(_fields, headers, PUT_TYPE) ;
}

inline string Veridic::Delete(DELETE &_fields, const string& endpoint) {
    return endpoint != DEFAULT ? post(_fields, endpoint, DELETE_TYPE) : post(_fields, DELETE_TYPE);
}

inline string Veridic::Delete(DELETE &_fields, const VHeaders  &headers, const string& endpoint) {
    return endpoint != DEFAULT ? post(_fields, headers, endpoint, DELETE_TYPE) : post(_fields, headers, DELETE_TYPE);
}

inline string Veridic::custom(POST &field, const string& type, const string& endpoint) {
    return endpoint != DEFAULT ?post(field, endpoint, type) : post(field, type);
}

inline string Veridic::custom(POST &fields, const VHeaders &headers, const string& type, const string& endpoint) {
    return endpoint != DEFAULT ?post(fields, headers , endpoint, type) : post(fields, type);
}