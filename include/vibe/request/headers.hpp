#ifndef VIBE_HEADERS_
#define VIBE_HEADERS_

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <stdexcept>

class HttpHeaders {
public:
    using Header = std::pair<std::string, std::string>;

    HttpHeaders() = default;

    HttpHeaders(std::initializer_list<Header> headers) {
        for (const auto& [key, value] : headers) {
            add(key, value);
        }
    }

    void add(std::string_view key, std::string_view value) {
        validate_header(key, value);
        headers.emplace_back(Header{std::string(key), std::string(value)});
    }
    std::string generate() const {
        std::string response;
        response.reserve(headers.size() * 32); // Reserva aproximada

        for (const auto& [key, value] : headers) {
            response += key;
            response += ": ";
            response += value;
            response += "\r\n";
        }

        return response;
    }


    template<typename... Args>
    static std::string Build(Args&&... args) {
        static_assert(sizeof...(Args) % 2 == 0,
            "Must provide even number of arguments as key-value pairs");

        HttpHeaders temp;
        temp.add_impl(std::forward<Args>(args)...);
        return temp.generate();
    }

private:
    std::vector<Header> headers;

    void validate_header(std::string_view key, std::string_view value) {
        if (key.empty() || value.empty()) {
            throw std::invalid_argument("Header key and value cannot be empty");
        }

        if (std::any_of(key.begin(), key.end(), [](char c) {
            return c == ':' || c == '\r' || c == '\n';
        })) {
            throw std::invalid_argument("Invalid characters in header key");
        }
    }

    template<typename... Args>
    void add_impl(Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            add_impl_helper(std::forward<Args>(args)...);
        }
    }

    template<typename Key, typename Value, typename... Rest>
    void add_impl_helper(Key&& key, Value&& value, Rest&&... rest) {
        add(std::forward<Key>(key), std::forward<Value>(value));
        add_impl(std::forward<Rest>(rest)...);
    }
};


#endif //VIBE_HEADERS_