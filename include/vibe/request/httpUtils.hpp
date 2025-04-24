#ifndef HTTP_UTILS_
#define HTTP_UTILS_

#include <sstream>
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>

class HttpUtils {

    static constexpr const char* SERVER_NAME = "Vibe/1.5";
    static constexpr const char* JSON_TYPE = "application/json; charset=utf-8";

    static inline const std::unordered_map<int, std::string> HTTP_STATUS_CODES = {
        {200, "200 OK"},
        {401, "401 Unauthorized"},
        {403, "403 Forbidden"},
        {429, "429 Too Many Requests"}
    };

public:
    HttpUtils() = delete;

    static std::string create_response(
        std::string_view content,
        std::string_view content_type,
        std::string_view additional_headers = "",
        int status_code = 200)
    {
        std::ostringstream response;

        // Encabezado HTTP
        response << "HTTP/1.1 " << get_status_message(status_code) << "\r\n"
                 << "Server: " << SERVER_NAME << "\r\n"
                 << "Content-Type: " << content_type << "\r\n"
                 << "Content-Length: " << content.size() << "\r\n"
                 << "Cache-Control: no-store, max-age=0\r\n"
                 << "Accept-Ranges: bytes\r\n"
                 << additional_headers
                 << "Connection: close\r\n"
                 << "\r\n"
                 << content;

        return response.str();
    }

    static std::string rate_limit_response(
        const std::chrono::milliseconds cooldown,
        const std::string_view message = "")
    {
        std::ostringstream json;
        std::string msg(message.empty() ?
            "Wait, this route has a " + std::to_string(cooldown.count()) + " second cooldown" :
            std::string(message));

        sanitize_json(msg);

        json << R"({"message": ")" << msg << R"(", "cooldown": )"
             << cooldown.count() << R"(, "status": "error"})";

        return create_response(
            json.str(),
            JSON_TYPE,
            "X-RateLimit-Limit: 1\r\n"
            "X-RateLimit-Remaining: 0\r\n"
            "X-RateLimit-Reset: " + std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count() + cooldown.count()) + "\r\n",
            429);
    }

    static std::optional<int> safe_string_to_int(std::string_view input) noexcept {
        try {
            size_t chars_processed;
            const int result = std::stoi(std::string(input), &chars_processed);

            if (chars_processed != input.size()) {
                return std::nullopt;
            }
            return result;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

private:
    static std::string_view get_status_message(int code) {
        if (auto it = HTTP_STATUS_CODES.find(code); it != HTTP_STATUS_CODES.end()) {
            return it->second;
        }
        return "500 Internal Server Error";
    }

    static void sanitize_json(std::string& str) {
        size_t pos = 0;
        while ((pos = str.find('"', pos))) {
            if (pos == std::string::npos) break;
            str.replace(pos, 1, "\\\"");
            pos += 2;
        }
    }
};


#endif //HTTP_UTILS_
