#ifndef VIBE_H
#define VIBE_H

#include "sockets.h"
#include "routes.hpp"
#include "config.hpp"
#include "request/router_epoll.h"
#include "util/enums.h"
#include <chrono>
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

    vibe::Config config_{};

    void tcpInt();
    // Re-applies the network-related fields of config_ to a live tcpControl.
    void applyNetworkConfig() noexcept;

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

    // ---- server configuration ----

    // Replaces the whole configuration (designated initializers recommended):
    //   router.configure({ .max_request_size = 64UL*1024*1024, .threads = 8 });
    Vibe& configure(const vibe::Config& config) noexcept;
    [[nodiscard]] const vibe::Config& config() const noexcept { return config_; }

    Vibe& setReadTimeout(std::chrono::milliseconds timeout) noexcept;
    Vibe& setWriteTimeout(std::chrono::milliseconds timeout) noexcept;
    Vibe& setMaxRequestSize(size_t bytes) noexcept;
    Vibe& setReadChunkSize(size_t bytes) noexcept;
    Vibe& setThreads(size_t threads) noexcept;
    Vibe& setMaxEvents(int max_events) noexcept;
    Vibe& setMaxQueueSize(size_t max_queue_size) noexcept;
    Vibe& setBacklog(int backlog) noexcept;
    Vibe& setBufferSize(int size) noexcept;
    // Toolchain used to compile readFileX templates:
    //   router.setCppToolchain({ .compiler = "g++-12", .standard = "c++20" });
    Vibe& setCppToolchain(const vibe::CppToolchain& toolchain) noexcept;

    int setPort(uint16_t) noexcept;
    [[nodiscard]] [[maybe_unused]] inline uint16_t getPort() const noexcept{
        constexpr uint16_t min_port = static_cast<uint16_t>(neo::MIN_PORT);
        constexpr uint16_t default_port = static_cast<uint16_t>(neo::DEF_PORT);
        return config_.port >= min_port ? config_.port : default_port;
    };
    void listen();
    void listenOne();
    void setListenStatus(neo::eStatus);

};

template <class T>
[[maybe_unused]] Vibe<T>::Vibe(const uint16_t port) {
    if (port >= neo::MIN_PORT) { config_.port = port; }
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
   router_epoll->getMainProcess(routes, neo::WHILE, config_);
}
template <class T>
void Vibe<T>::listenOne() {
    router_epoll->getMainProcess(routes, neo::UNIQUE, config_);
}

template <class T>
void Vibe<T>::setListenStatus(neo::eStatus _status) {
    router_epoll->setListenStatus(_status);
}


template <class T>
int Vibe<T>::setPort(const uint16_t _port) noexcept {
    if (_port >= neo::MIN_PORT) {
        config_.port = _port;
        if(tcpControl != nullptr) {
            tcpControl->setPort(config_.port);
            return neo::OK;
        }
    }
    return neo::ERROR;
}


template <class T>
Vibe<T>& Vibe<T>::configure(const vibe::Config& config) noexcept {
    const uint16_t previous_port = config_.port;
    config_ = config;
    // A zero/default port in the new config means "keep the current one";
    // otherwise router.setPort(8080); router.configure({...}) would silently
    // move the server back to the default port.
    if (config_.port < static_cast<uint16_t>(neo::MIN_PORT))
        config_.port = previous_port;
    applyNetworkConfig();
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setReadTimeout(const std::chrono::milliseconds timeout) noexcept {
    config_.read_timeout = timeout;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setWriteTimeout(const std::chrono::milliseconds timeout) noexcept {
    config_.write_timeout = timeout;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setMaxRequestSize(const size_t bytes) noexcept {
    config_.max_request_size = bytes;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setReadChunkSize(const size_t bytes) noexcept {
    config_.read_chunk = bytes == 0 ? 1 : bytes;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setThreads(const size_t threads) noexcept {
    config_.threads = threads;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setMaxEvents(const int max_events) noexcept {
    config_.max_events = max_events;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setMaxQueueSize(const size_t max_queue_size) noexcept {
    config_.max_queue_size = max_queue_size;
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setBacklog(const int backlog) noexcept {
    config_.backlog = backlog;
    if (tcpControl != nullptr)
        tcpControl->setSessions(backlog);
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setBufferSize(const int size) noexcept {
    config_.buffer_size = size;
    if (tcpControl != nullptr)
        tcpControl->setBuffer(size);
    return *this;
}

template <class T>
Vibe<T>& Vibe<T>::setCppToolchain(const vibe::CppToolchain& toolchain) noexcept {
    config_.render.cpp = toolchain;
    return *this;
}

template <class T>
void Vibe<T>::applyNetworkConfig() noexcept {
    if (tcpControl == nullptr)
        return;
    constexpr uint16_t min_port = static_cast<uint16_t>(neo::MIN_PORT);
    constexpr uint16_t default_port = static_cast<uint16_t>(neo::DEF_PORT);
    tcpControl->setBuffer(config_.buffer_size);
    tcpControl->setPort(config_.port >= min_port ? config_.port : default_port);
    tcpControl->setSessions(config_.backlog);
}


template<class T>
void Vibe<T>::tcpInt() {

    tcpControl = make_shared<T>();
    applyNetworkConfig();

    routes = make_shared<RoutesMap>();
    router_epoll = make_shared<workers::RouterEpoll<T>>(tcpControl);
}
using Router  = Vibe<Server>;
using Convert = utility_t;


#endif // VIBE_H