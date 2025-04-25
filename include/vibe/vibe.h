#ifndef VIBE_H
#define VIBE_H

#include "sockets.h"
#include "request/routes.hpp"
#include "request/router_epoll.h"
#include "abstract.hpp"
#include "configuration.hpp"
#include "request/httpUtils.hpp"
#include <memory>
#include <string>

using std::make_shared, std::make_unique;
using std::string;

using workers::RoutesMap;
using enums::neo;

template <class T>
class Vibe {

    shared_ptr<workers::RouterEpoll<T>> router_epoll;
    shared_ptr<RoutesMap> routes;
    std::shared_ptr<T> tcpControl;

    uint16_t PORT{neo::DEF_PORT};

    void tcpInt();

public:
    [[maybe_unused]] explicit Vibe(uint16_t port);
    explicit Vibe();

    int http_response(const string&, MiddlewareList, const string&);

    int get(const string& route,    const MiddlewareList& middlewares);
    int post(const string& route,   const MiddlewareList& middlewares);
    int put(const string& route,    const MiddlewareList& middlewares);
    int deleteX(const string& route,const MiddlewareList& middlewares);
    int patch(const string& route,  const MiddlewareList& middlewares);
    int head(const string& route,   const MiddlewareList& middlewares);
    int options(const string& route,const MiddlewareList& middlewares);
    int link(const string& route,   const MiddlewareList& middlewares);
    int unlink(const string& route, const MiddlewareList& middlewares);
    int purge(const string& route,  const MiddlewareList& middlewares);

    int use(const Route_t&);

    int setPort(uint16_t) noexcept;
    [[nodiscard]] [[maybe_unused]] inline uint16_t getPort() const noexcept{return PORT;};
    void listen();
    void listenOne();
    void setListenStatus(neo::eStatus);

};

template <class T>
[[maybe_unused]] Vibe<T>::Vibe(const uint16_t port) {
    if (port >= neo::MIN_PORT) { PORT = port; }
    tcpInt();
}

template <class T>
Vibe<T>::Vibe() {
    tcpInt();
}

template <class T>

int Vibe<T>::http_response(const string &endpoint, MiddlewareList middlewareList, const string& type) {
    try {
        if (routes == nullptr)
            routes = make_shared<RoutesMap>();

        routes->operator[](endpoint + type) = make_unique<listen_routes>( endpoint, std::move(middlewareList), type);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return neo::ERROR;
    }
    return neo::OK;
}

template <class T>
[[maybe_unused]] int Vibe<T>::get(const string& route,const MiddlewareList &middlewares){
    return http_response(route, middlewares, GET_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::post(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, POST_TYPE );
}
template <class T>
[[maybe_unused]] int Vibe<T>::put(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, PUT_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::deleteX(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, DELETE_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::patch(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, PATCH_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::head(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, HEAD_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::options(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, OPTIONS_TYPE);
}
template <class T>
[[maybe_unused]]  int Vibe<T>::link(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, LINK_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::unlink(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, UNLINK_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::purge(const string& route,const MiddlewareList &middlewares) {
    return http_response(route, middlewares, PURGE_TYPE);
}

template <class T>
[[maybe_unused]] int Vibe<T>::use(const Route_t & route) {
    return http_response( route.route, route.middlewares, route.type);
}


template <class T>
void Vibe<T>::listen() {
   router_epoll->getMainProcess(routes);
}
template <class T>
void Vibe<T>::listenOne() {
    router_epoll->getMainProcess(routes, neo::UNIQUE);
}

template <class T>
void Vibe<T>::setListenStatus(neo::eStatus _status) {
    router_epoll->setListenStatus(_status);
}


template <class T>
int Vibe<T>::setPort(const uint16_t _port) noexcept {
    if (_port >= neo::MIN_PORT) {
        PORT = _port;
        if(tcpControl != nullptr) {
            tcpControl->setPort(PORT);
            return neo::OK;
        }
    }
    return neo::ERROR;
}


template<class T>
void Vibe<T>::tcpInt() {

    tcpControl = make_shared<T>();
    tcpControl->setBuffer(BUFFER);
    tcpControl->setPort(PORT);
    tcpControl->setSessions(SESSION);

    routes = make_shared<RoutesMap>();
    router_epoll = make_shared<workers::RouterEpoll<T>>(tcpControl);
}
using Router  = Vibe<Server>;
using Convert = HttpUtils;


#endif // VIBE_H