//
// Modern HTTP/1.1 request parsing (C++20): zero-copy scanning with
// std::string_view, percent-decoding, query/form parsing and
// multipart/form-data (file upload) support.
//

#ifndef VIBE_HTTP_MESSAGE_HPP
#define VIBE_HTTP_MESSAGE_HPP

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vibe::http {

    inline constexpr std::string_view TYPE_FORM_URLENCODED = "application/x-www-form-urlencoded";
    inline constexpr std::string_view TYPE_JSON            = "application/json";
    inline constexpr std::string_view TYPE_MULTIPART       = "multipart/form-data";
    inline constexpr std::string_view TYPE_TEXT            = "text/plain";

    // An uploaded file extracted from a multipart/form-data body.
    struct UploadedFile {
        std::string field;        // form field name (Content-Disposition name="...")
        std::string filename;     // original file name sent by the client
        std::string mime;         // part Content-Type (application/octet-stream by default)
        std::string content;      // raw bytes

        [[nodiscard]] size_t size()  const noexcept { return content.size(); }
        [[nodiscard]] bool   empty() const noexcept { return content.empty(); }

        bool save_to(const std::string& path) const {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open())
                return false;
            out.write(content.data(), static_cast<std::streamsize>(content.size()));
            return out.good();
        }
    };

    namespace detail {

        [[nodiscard]] inline char ascii_lower(const char c) noexcept {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        [[nodiscard]] inline bool iequals(const std::string_view a, const std::string_view b) noexcept {
            return a.size() == b.size()
                && std::equal(a.begin(), a.end(), b.begin(), [](const char x, const char y) {
                       return ascii_lower(x) == ascii_lower(y);
                   });
        }

        [[nodiscard]] inline std::string_view trim(const std::string_view sv) noexcept {
            constexpr std::string_view WS = " \t\r\n";
            const size_t first = sv.find_first_not_of(WS);
            if (first == std::string_view::npos)
                return {};
            return sv.substr(first, sv.find_last_not_of(WS) - first + 1);
        }

        [[nodiscard]] inline std::string lower(const std::string_view sv) {
            std::string out;
            out.reserve(sv.size());
            for (const char c : sv)
                out.push_back(ascii_lower(c));
            return out;
        }

        [[nodiscard]] inline int hex_value(const char c) noexcept {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

    } // namespace detail

    // Decodes application/x-www-form-urlencoded escapes: %XX and '+'.
    [[nodiscard]] inline std::string url_decode(const std::string_view in) {
        std::string out;
        out.reserve(in.size());

        for (size_t i = 0; i < in.size(); ++i) {
            const char c = in[i];
            if (c == '%' && i + 2 < in.size() + 1 && i + 2 <= in.size() - 1) {
                const int hi = detail::hex_value(in[i + 1]);
                const int lo = detail::hex_value(in[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    out.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                    continue;
                }
                out.push_back(c); // malformed escape: keep literally
            }
            else if (c == '+') {
                out.push_back(' ');
            }
            else {
                out.push_back(c);
            }
        }
        return out;
    }

    // Parses "a=1&b=two" (query strings and urlencoded bodies) with decoding.
    [[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
    parse_query(const std::string_view raw) {
        std::vector<std::pair<std::string, std::string>> out;

        size_t pos = 0;
        while (pos <= raw.size()) {
            const size_t amp = raw.find('&', pos);
            const std::string_view segment = raw.substr(pos, amp == std::string_view::npos ? amp : amp - pos);

            if (!segment.empty()) {
                if (const size_t eq = segment.find('='); eq != std::string_view::npos)
                    out.emplace_back(url_decode(segment.substr(0, eq)), url_decode(segment.substr(eq + 1)));
                else
                    out.emplace_back(url_decode(segment), std::string{});
            }

            if (amp == std::string_view::npos)
                break;
            pos = amp + 1;
        }
        return out;
    }

    // A fully parsed HTTP/1.1 request message.
    class Message {
    public:
        std::string method;        // GET, POST, ...
        std::string target;        // raw request target, e.g. /index?id=2
        std::string path;          // target without query string or trailing '/'
        std::string query;         // raw query string (still percent-encoded)
        std::string version;       // e.g. HTTP/1.1
        std::string body;          // message body (bounded by Content-Length when present)

        // Header list preserving wire order and original name casing.
        std::vector<std::pair<std::string, std::string>> headers;

        [[nodiscard]] std::string_view header(const std::string_view name) const noexcept {
            for (const auto& [hname, hvalue] : headers)
                if (detail::iequals(hname, name))
                    return hvalue;
            return {};
        }

        [[nodiscard]] bool has_header(const std::string_view name) const noexcept {
            return std::any_of(headers.begin(), headers.end(), [&](const auto& h) {
                return detail::iequals(h.first, name);
            });
        }

        // Lowercased media type without parameters: "text/html; charset=x" -> "text/html".
        [[nodiscard]] std::string_view content_type() const noexcept { return content_type_; }

        // Value of a Content-Type parameter, e.g. content_type_param("boundary").
        [[nodiscard]] std::optional<std::string_view> content_type_param(const std::string_view key) const noexcept {
            for (const auto& [k, v] : content_type_params_)
                if (k == key)
                    return v;
            return std::nullopt;
        }

        [[nodiscard]] size_t content_length() const noexcept {
            const std::string_view value = header("Content-Length");
            size_t length = 0;
            const auto* first = value.data();
            const auto* last  = first + value.size();
            if (const auto [ptr, ec] = std::from_chars(first, last, length);
                ec == std::errc{} && ptr != first)
                return length;
            return 0;
        }

        static std::optional<Message> parse(const std::string_view raw) {
            if (raw.empty())
                return std::nullopt;

            Message msg;
            size_t cursor = 0;

            // ---- request line: METHOD SP TARGET SP VERSION ----
            const size_t line_end = find_line_end(raw, 0);
            const std::string_view request_line = raw.substr(0, line_end);

            const size_t sp1 = request_line.find(' ');
            if (sp1 == std::string_view::npos || sp1 == 0)
                return std::nullopt;

            const size_t sp2 = request_line.find(' ', sp1 + 1);
            msg.method  = std::string(request_line.substr(0, sp1));
            msg.target  = std::string(request_line.substr(sp1 + 1, sp2 == std::string_view::npos ? sp2 : sp2 - sp1 - 1));
            msg.version = sp2 == std::string_view::npos
                              ? std::string{}
                              : std::string(detail::trim(request_line.substr(sp2 + 1)));

            if (msg.target.empty())
                return std::nullopt;

            // path / query split; strip a single trailing '/' like the legacy router did
            if (const size_t q = msg.target.find('?'); q != std::string::npos) {
                msg.path  = msg.target.substr(0, q);
                msg.query = msg.target.substr(q + 1);
            } else {
                msg.path = msg.target;
            }
            if (msg.path.size() > 1 && msg.path.back() == '/')
                msg.path.pop_back();

            cursor = advance_past_eol(raw, line_end);

            // ---- header block ----
            while (cursor < raw.size()) {
                const size_t end = find_line_end(raw, cursor);
                std::string_view line = raw.substr(cursor, end - cursor);
                if (!line.empty() && line.back() == '\r')
                    line.remove_suffix(1);

                if (line.empty()) { // blank line: body follows
                    cursor = advance_past_eol(raw, end);
                    break;
                }

                if (const size_t colon = line.find(':'); colon != std::string_view::npos && colon > 0)
                    msg.headers.emplace_back(std::string(detail::trim(line.substr(0, colon))),
                                             std::string(detail::trim(line.substr(colon + 1))));

                if (end >= raw.size()) { // no blank line: request ends after headers
                    cursor = raw.size();
                    break;
                }
                cursor = advance_past_eol(raw, end);
            }

            // ---- body (bounded by Content-Length when the client sent one) ----
            std::string_view body = raw.substr(std::min(cursor, raw.size()));
            if (const size_t declared = msg.content_length(); declared > 0 && declared < body.size())
                body = body.substr(0, declared);
            msg.body = std::string(body);

            msg.parse_content_type();
            return msg;
        }

    private:
        std::string content_type_storage_;
        std::string_view content_type_;
        std::vector<std::pair<std::string, std::string>> content_type_params_;

        static size_t find_line_end(const std::string_view raw, const size_t from) noexcept {
            const size_t nl = raw.find('\n', from);
            return nl == std::string_view::npos ? raw.size() : nl;
        }

        static size_t advance_past_eol(const std::string_view raw, const size_t line_end) noexcept {
            return line_end >= raw.size() ? raw.size() : line_end + 1;
        }

        void parse_content_type() {
            const std::string_view value = header("Content-Type");
            if (value.empty())
                return;

            const size_t semi = value.find(';');
            content_type_storage_ = detail::lower(detail::trim(value.substr(0, semi)));
            content_type_ = content_type_storage_;

            // parse "; key=value" parameters (boundary, charset, ...)
            size_t pos = semi == std::string_view::npos ? value.size() : semi + 1;
            while (pos < value.size()) {
                const size_t next = value.find(';', pos);
                const std::string_view pair = value.substr(pos, next == std::string_view::npos ? next : next - pos);
                if (const size_t eq = pair.find('='); eq != std::string_view::npos) {
                    std::string_view val = detail::trim(pair.substr(eq + 1));
                    if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                        val = val.substr(1, val.size() - 2);
                    content_type_params_.emplace_back(detail::lower(detail::trim(pair.substr(0, eq))),
                                                      std::string(val));
                }
                if (next == std::string_view::npos)
                    break;
                pos = next + 1;
            }
        }
    };

    // Result of parsing a multipart/form-data body.
    struct Multipart {
        std::vector<std::pair<std::string, std::string>> fields;
        std::vector<UploadedFile> files;
    };

    namespace detail {

        // Extracts a quoted token from a Content-Disposition header value:
        // disposition_param(R"(form-data; name="doc"; filename="a.txt")", "name") -> "doc"
        [[nodiscard]] inline std::string disposition_param(const std::string_view disposition,
                                                           const std::string_view key) {
            const std::string needle = std::string(key) + "=\"";
            const size_t begin = disposition.find(needle);
            if (begin == std::string_view::npos)
                return {};
            const size_t value_begin = begin + needle.size();
            const size_t value_end = disposition.find('"', value_begin);
            if (value_end == std::string_view::npos)
                return {};
            return std::string(disposition.substr(value_begin, value_end - value_begin));
        }

    } // namespace detail

    // Splits a multipart/form-data body into plain fields and uploaded files.
    [[nodiscard]] inline Multipart parse_multipart(const std::string_view body,
                                                   const std::string_view boundary) {
        Multipart result;
        if (boundary.empty())
            return result;

        const std::string delimiter = "--" + std::string(boundary);
        size_t pos = body.find(delimiter);
        if (pos == std::string_view::npos)
            return result;
        pos += delimiter.size();

        while (pos <= body.size()) {
            // Final delimiter ("--boundary--") ends the message.
            if (body.substr(pos, 2) == "--")
                break;
            if (body.substr(pos, 2) == "\r\n")
                pos += 2;
            else if (body.substr(pos, 1) == "\n")
                pos += 1;

            const size_t next = body.find(delimiter, pos);
            if (next == std::string_view::npos)
                break;

            std::string_view part = body.substr(pos, next - pos);
            pos = next + delimiter.size();

            // Strip the CRLF that precedes the next delimiter.
            if (part.size() >= 2 && part.substr(part.size() - 2) == "\r\n")
                part.remove_suffix(2);
            else if (!part.empty() && part.back() == '\n')
                part.remove_suffix(1);

            const size_t split = part.find("\r\n\r\n");
            const std::string_view part_headers = split == std::string_view::npos ? part : part.substr(0, split);
            const std::string_view content = split == std::string_view::npos
                                                 ? std::string_view{}
                                                 : part.substr(split + 4);

            std::string disposition, part_mime;
            size_t hpos = 0;
            while (hpos <= part_headers.size()) {
                const size_t eol = part_headers.find("\r\n", hpos);
                const std::string_view line = part_headers.substr(hpos, eol == std::string_view::npos ? eol : eol - hpos);
                if (const size_t colon = line.find(':'); colon != std::string_view::npos) {
                    const std::string_view hname = detail::trim(line.substr(0, colon));
                    const std::string_view hvalue = detail::trim(line.substr(colon + 1));
                    if (detail::iequals(hname, "Content-Disposition"))
                        disposition = std::string(hvalue);
                    else if (detail::iequals(hname, "Content-Type"))
                        part_mime = std::string(hvalue);
                }
                if (eol == std::string_view::npos)
                    break;
                hpos = eol + 2;
            }

            std::string name = detail::disposition_param(disposition, "name");
            if (name.empty())
                continue;

            if (std::string filename = detail::disposition_param(disposition, "filename"); !filename.empty()) {
                UploadedFile file;
                file.field    = std::move(name);
                file.filename = std::move(filename);
                file.mime     = part_mime.empty() ? "application/octet-stream" : std::move(part_mime);
                file.content  = std::string(content);
                result.files.push_back(std::move(file));
            } else {
                result.fields.emplace_back(std::move(name), std::string(content));
            }
        }
        return result;
    }

} // namespace vibe::http

#endif // VIBE_HTTP_MESSAGE_HPP
