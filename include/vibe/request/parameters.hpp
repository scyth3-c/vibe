#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <charconv>
#include <chrono>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "../http/response.hpp"

using std::string;
using std::vector;

class param_box {
    std::pair<string, string> _body;

public:
    string name;
    string value;

    param_box(string, string);
    explicit param_box(std::pair<string, string> content) : _body(content) {
        name  = std::move(content.first);
        value = std::move(content.second);
    }

    // Typed conversion: params.get("id").as<int>() / as<double>() / as<bool>() ...
    template <class T>
    [[nodiscard]] T as(T fallback = {}) const {
        if constexpr (std::is_same_v<T, string>) {
            return value;
        } else if constexpr (std::is_same_v<T, std::string_view>) {
            return value;
        } else if constexpr (std::is_same_v<T, const char*>) {
            return value.c_str();
        } else if constexpr (std::is_same_v<T, bool>) {
            return value == "1" || value == "true" || value == "on" || value == "yes"
                    ? true
                    : (value == "0" || value == "false" || value == "off" || value == "no" ? false : fallback);
        } else if constexpr (std::is_arithmetic_v<T>) {
            T out{};
            const char* first = value.data();
            const char* last  = first + value.size();
            if (const auto [ptr, ec] = std::from_chars(first, last, out);
                ec == std::errc{} && ptr == last)
                return out;
            return fallback;
        } else {
            static_assert(std::is_arithmetic_v<T>, "param_box::as<T> supports string, string_view and arithmetic types");
        }
    }

    [[nodiscard]] bool empty() const noexcept { return value.empty(); }
};


class Param_t {
    vector<std::pair<string, string>> _list;

public:
    Param_t() = default;
    ~Param_t() { _list.clear(); }

    [[maybe_unused]] explicit Param_t(vector<std::pair<string, string>> list) : _list(std::move(list)) {}

    param_box operator[](int);

    void setContent(const vector<std::pair<string, string>>&);

    [[maybe_unused]] inline void clear() { _list.clear(); }
    [[maybe_unused]] [[nodiscard]] inline bool empty() const { return _list.empty(); }

    [[maybe_unused]] bool exist(const string&);
    [[nodiscard]] bool exist(std::string_view name) const;

    [[maybe_unused]] param_box get(const string&);

    // Value of `name` or `fallback` when missing.
    [[maybe_unused]] [[nodiscard]] string value_or(std::string_view name, string fallback) const;

    [[nodiscard]] inline vector<std::pair<string, string>> toArray() const noexcept { return _list; }
    [[nodiscard]] inline size_t size() const noexcept { return _list.size(); }
};


struct utility_t {
    static string prepare_basic(const string& _txt,
                                const string& _type,
                                const string& headers,
                                const string& status = "200") {

        vibe::http::Response response;
        response.status(status_code_of(status)).type(_type);

        // The legacy API hands over headers as a raw "Name: value\n..." block.
        size_t pos = 0;
        while (pos < headers.size()) {
            const size_t eol = headers.find('\n', pos);
            const std::string_view line = std::string_view(headers).substr(
                pos, eol == string::npos ? eol : eol - pos);

            if (const size_t colon = line.find(':'); colon != std::string_view::npos && colon > 0) {
                std::string_view value = line.substr(colon + 1);
                if (!value.empty() && value.front() == ' ')
                    value.remove_prefix(1);
                if (!value.empty() && value.back() == '\r')
                    value.remove_suffix(1);
                response.set(line.substr(0, colon), value);
            }

            if (eol == string::npos)
                break;
            pos = eol + 1;
        }

        response.body(_txt);
        return response.str();
    }

    [[maybe_unused]] static string guard_route(const std::chrono::duration<double>::rep seconds,
                                               string msg = "") {

        const string body = R"lit({"message":")lit"
            + string(not msg.empty()
                         ? std::move(msg)
                         : "wait, this route has a " + std::to_string(static_cast<int>(seconds)) + " second cooldown")
            + R"lit("})lit";

        return prepare_basic(body, "application/json", "", "401");
    }

    [[maybe_unused]] static int toInt(const string& data) {
        int result = 0;
        const char* first = data.data();
        const char* last  = first + data.size();
        if (const auto [ptr, ec] = std::from_chars(first, last, result);
            ec == std::errc{} && ptr == last)
            return result;
        return 0;
    }

private:
    // Legacy code passes the status as a string ("200", "404", "200 OK").
    static int status_code_of(const string& status) {
        int code = 200;
        const char* first = status.data();
        const char* last  = first + status.size();
        if (const auto [ptr, ec] = std::from_chars(first, last, code);
            ec == std::errc{} && ptr != first)
            return code;
        return 200;
    }
};


[[maybe_unused]] constexpr auto ERROR_GET = "HTTP/1.1 404 BAD\n"
                           "Server: Vibe/1.0\n"
                           "Content-Type: application/json\n"
                           "Cache-Control: Expires"
                           "Content-Length: 37\n"
                           "Accept-Ranges: bytes\n"
                           "Connection: close\n"
                           "\n"
                           R"lit({"error":"this route is not defined"})lit";


#endif // PARAMETERS_HPP
