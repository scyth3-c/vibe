#pragma once
#ifndef ROUTES_DEMAND_HPP
#define ROUTES_DEMAND_HPP

#include <string>

#include <utility>
#include <vector>
#include <initializer_list>
#include <functional>
#include <future>
#include <chrono>

#include "request.hpp"

#include "../files/basic_render.h"
#include "../files/cpp_reader.h"
#include "../files/mg_reader.h"
#include "../files/data_render.h"
#include "../files/json_legacy.hpp"
#include "../request/httpUtils.hpp"
#include "../request/headers.hpp"

using std::string;
using namespace std::chrono;

struct Route {
private:
    string route_name{},
           route_type{};
    bool lock{};
    int timeline{};

public:
    [[maybe_unused]] explicit Route(string _route) : route_name(std::move(_route)), lock(false), timeline(0) {}
    Route() = default;

    inline Route& operator = (string _val) { route_name = std::move(_val); return *this; }
    inline void setType(string _type) { route_type = std::move(_type); }
    [[nodiscard]] inline string getName () const noexcept { return route_name; }
    [[nodiscard]] inline string getType () const noexcept { return route_type; }
};

class Query {

    bool next_enable  { false};
    milliseconds time_key    {0};

    string response {"default"},
           responseBody,
           headers, guardMsg{};

    public:
    Query()  = default;
    ~Query() = default;

    Request body; // property access
    Request query;

    [[nodiscard]] string  getData()    const noexcept;
    [[nodiscard]] bool    getNext()    const noexcept;

    [[maybe_unused]] void    next()       noexcept;

    void    lock()        noexcept;
    [[nodiscard]] milliseconds    getTimeKey()  const noexcept;
    [[nodiscard]] string  getGuardMsg() const noexcept;

    [[maybe_unused]] void    setHeaders(const HttpHeaders&) noexcept;
    [[maybe_unused]] void    setHeaders(const string&) noexcept;
    [[maybe_unused]] void    guard(const milliseconds&, string custom_msg="") noexcept;


    //  PARAMS:  CONTENT OPTIONAL CALLBACK

    [[maybe_unused]] void    json(const string&, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    html(const string&, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    send(const string&, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    readFile(const string&,const string&, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    readFileX(const string&,const string&, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    compose(const string&,int, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    render(const string&, const std::function<dataRender(dataRender&)>& callback=[](dataRender&)->dataRender{ return dataRender(nullptr); }) noexcept;

    // PARAMS:  CONTENT  STATUS OPTIONAL CALLBACK
    [[maybe_unused]] void    json(const string&, int, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    html(const string&, int, const std::function<void()>& callback=[]()->void{}) noexcept;
    [[maybe_unused]] void    send(const string&, int, const std::function<void()>& callback=[]()->void{}) noexcept;

};


template <class... P>
struct Core_init_t  {

    [[maybe_unused]] Core_init_t(std::initializer_list<P...> list) :
                functions(std::move(list)) {}
    [[maybe_unused]] Core_init_t() = default;

    std::vector<P...> functions;
    Query *remote_control{};

    [[nodiscard]] [[maybe_unused]] inline size_t size() const noexcept { return functions.size(); }

     std::pair<string, milliseconds> execute(string _raw, string headers, std::unique_ptr<string> &guard_msg) {
        remote_control = new Query();

        string response{};

        remote_control->body.clear_parameters();
        remote_control->body.setRawParametersData(std::move(_raw));
        remote_control->body.setRawHeadersData(std::move(headers));

        // middlewares execution
        for (size_t i = 0; i < functions.size(); i++) {
            remote_control->lock();

            functions[i](*remote_control);
            if(remote_control->getNext())
                continue;
            else
                break;
        }

        response += remote_control->getData();
        milliseconds time_key = remote_control->getTimeKey();
        if(time_key > milliseconds(0))
            guard_msg = std::make_unique<string>(remote_control->getGuardMsg());

        delete remote_control;
        return {response, time_key};
    }
};

using MiddlewareList = Core_init_t<std::function<void(Query&)>> ;

struct listen_routes {

    listen_routes(string _route, MiddlewareList _funcs, string _type) : middlewares(std::move(_funcs)){
        route = std::move(_route);
        route.setType(std::move(_type));
        time_key = milliseconds(0);
    }

    Route route;
    MiddlewareList middlewares;
    milliseconds time_key{};
    system_clock::time_point time_point{};
    std::unique_ptr<string> guardRouteMsg = nullptr;
};

struct Route_t {
    Route_t(string _r, MiddlewareList _m, string _t) : route(std::move(_r)), middlewares(std::move(_m)), type(std::move(_t)) { }
    Route_t()= default;
    string route;
    MiddlewareList middlewares;
    string type;

};




#endif /*ROUTES_DEMAND_HPP */