//
// Fluent HTTP/1.1 response builder: proper CRLF framing, reason phrases
// and automatic Content-Length. Replaces the legacy string-concatenation
// style of hand-written HTTP responses.
//

#ifndef VERMELL_HTTP_RESPONSE_HPP
#define VERMELL_HTTP_RESPONSE_HPP

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vermell::http {

    class Response {
    public:
        Response() = default;

        Response& status(const int code) noexcept {
            code_ = code;
            return *this;
        }

        Response& type(const std::string_view mime) {
            return set("Content-Type", mime);
        }

        // Adds or replaces (case-insensitive) a header. CR/LF and other
        // control characters are stripped from both name and value so a
        // tainted string can never split the response into extra headers.
        Response& set(const std::string_view name, const std::string_view value) {
            const std::string safe_name  = sanitize(name);
            const std::string safe_value = sanitize(value);
            if (safe_name.empty())
                return *this;
            for (auto& [hname, hvalue] : headers_) {
                if (iequals(hname, safe_name)) {
                    hvalue = safe_value;
                    return *this;
                }
            }
            headers_.emplace_back(safe_name, safe_value);
            return *this;
        }

        Response& body(std::string content) {
            body_ = std::move(content);
            return *this;
        }

        [[nodiscard]] int status() const noexcept { return code_; }
        [[nodiscard]] const std::string& body() const noexcept { return body_; }

        [[nodiscard]] bool has(const std::string_view name) const noexcept {
            return std::any_of(headers_.begin(), headers_.end(), [&](const auto& h) {
                return iequals(h.first, name);
            });
        }

        // Serializes the response with correct CRLF framing.
        [[nodiscard]] std::string str() const {
            std::string out;
            out.reserve(128 + body_.size());

            out += "HTTP/1.1 ";
            out += std::to_string(code_);
            out += ' ';
            out += reason(code_);
            out += "\r\n";

            if (!has("Server"))
                out += "Server: Vermell/1.0\r\n";

            if (!has("Content-Type"))
                out += "Content-Type: text/plain\r\n";

            for (const auto& [name, value] : headers_) {
                out += name;
                out += ": ";
                out += value;
                out += "\r\n";
            }

            if (!has("Content-Length")) {
                out += "Content-Length: ";
                out += std::to_string(body_.size());
                out += "\r\n";
            }

            if (!has("Accept-Ranges"))
                out += "Accept-Ranges: bytes\r\n";

            if (!has("Connection"))
                out += "Connection: close\r\n";

            out += "\r\n";
            out += body_;
            return out;
        }

        // RFC 7231 reason phrases ("OK" fallback mirrors the legacy behavior).
        [[nodiscard]] static std::string_view reason(const int code) noexcept {
            switch (code) {
                case 100: return "Continue";
                case 101: return "Switching Protocols";
                case 200: return "OK";
                case 201: return "Created";
                case 202: return "Accepted";
                case 203: return "Non-Authoritative Information";
                case 204: return "No Content";
                case 205: return "Reset Content";
                case 206: return "Partial Content";
                case 300: return "Multiple Choices";
                case 301: return "Moved Permanently";
                case 302: return "Found";
                case 303: return "See Other";
                case 304: return "Not Modified";
                case 307: return "Temporary Redirect";
                case 308: return "Permanent Redirect";
                case 400: return "Bad Request";
                case 401: return "Unauthorized";
                case 402: return "Payment Required";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 406: return "Not Acceptable";
                case 408: return "Request Timeout";
                case 409: return "Conflict";
                case 410: return "Gone";
                case 411: return "Length Required";
                case 413: return "Payload Too Large";
                case 414: return "URI Too Long";
                case 415: return "Unsupported Media Type";
                case 418: return "I'm a teapot";
                case 422: return "Unprocessable Content";
                case 429: return "Too Many Requests";
                case 431: return "Request Header Fields Too Large";
                case 500: return "Internal Server Error";
                case 501: return "Not Implemented";
                case 502: return "Bad Gateway";
                case 503: return "Service Unavailable";
                case 504: return "Gateway Timeout";
                default:  return "OK";
            }
        }

    private:
        static bool iequals(const std::string_view a, const std::string_view b) noexcept {
            return a.size() == b.size()
                && std::equal(a.begin(), a.end(), b.begin(), [](const char x, const char y) {
                       return std::tolower(static_cast<unsigned char>(x))
                           == std::tolower(static_cast<unsigned char>(y));
                   });
        }

        // Truncates at the first CR/LF or C0 control character (HTTP
        // response splitting): everything from there on is discarded.
        static std::string sanitize(const std::string_view in) {
            std::string out;
            out.reserve(in.size());
            for (const char c : in) {
                if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f)
                    break;
                out.push_back(c);
            }
            return out;
        }

        int code_ = 200;
        std::vector<std::pair<std::string, std::string>> headers_;
        std::string body_;
    };

} // namespace vermell::http

#endif // VERMELL_HTTP_RESPONSE_HPP
