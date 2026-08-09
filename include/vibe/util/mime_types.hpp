//
// MIME type registry: maps file extensions to Content-Type values.
//

#ifndef VIBE_MIME_TYPES_HPP
#define VIBE_MIME_TYPES_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace vibe::mime {

    inline constexpr std::string_view DEFAULT_MIME = "application/octet-stream";

    namespace detail {
        inline const std::unordered_map<std::string_view, std::string_view>& table() {
            static const std::unordered_map<std::string_view, std::string_view> TABLE = {
                // text
                {"html",  "text/html"},
                {"htm",   "text/html"},
                {"css",   "text/css"},
                {"csv",   "text/csv"},
                {"txt",   "text/plain"},
                {"md",    "text/markdown"},
                {"xml",   "application/xml"},
                // application
                {"json",  "application/json"},
                {"js",    "application/javascript"},
                {"mjs",   "application/javascript"},
                {"pdf",   "application/pdf"},
                {"zip",   "application/zip"},
                {"gz",    "application/gzip"},
                {"tar",   "application/x-tar"},
                {"rar",   "application/vnd.rar"},
                {"7z",    "application/x-7z-compressed"},
                {"wasm",  "application/wasm"},
                {"bin",   "application/octet-stream"},
                {"doc",   "application/msword"},
                {"docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
                {"xls",   "application/vnd.ms-excel"},
                {"xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
                {"ppt",   "application/vnd.ms-powerpoint"},
                {"pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
                // images
                {"png",   "image/png"},
                {"jpg",   "image/jpeg"},
                {"jpeg",  "image/jpeg"},
                {"gif",   "image/gif"},
                {"bmp",   "image/bmp"},
                {"svg",   "image/svg+xml"},
                {"ico",   "image/x-icon"},
                {"webp",  "image/webp"},
                {"avif",  "image/avif"},
                {"tif",   "image/tiff"},
                {"tiff",  "image/tiff"},
                // audio / video
                {"mp3",   "audio/mpeg"},
                {"wav",   "audio/wav"},
                {"ogg",   "audio/ogg"},
                {"mp4",   "video/mp4"},
                {"m4v",   "video/mp4"},
                {"webm",  "video/webm"},
                {"mpeg",  "video/mpeg"},
                {"mpg",   "video/mpeg"},
                {"mov",   "video/quicktime"},
                // fonts
                {"ttf",   "font/ttf"},
                {"otf",   "font/otf"},
                {"woff",  "font/woff"},
                {"woff2", "font/woff2"},
                {"eot",   "application/vnd.ms-fontobject"},
                // misc
                {"ics",   "text/calendar"},
                {"sh",    "application/x-sh"},
                {"php",   "application/x-httpd-php"},
            };
            return TABLE;
        }
    } // namespace detail

    // Returns the MIME type for a path or bare extension ("png", "a/b/pic.jpg").
    // Unknown or missing extensions resolve to application/octet-stream.
    [[nodiscard]] inline std::string_view of(std::string_view path) {
        const size_t dot = path.find_last_of('.');
        if (dot == std::string_view::npos || dot + 1 >= path.size())
            return DEFAULT_MIME;

        // No slash may follow the dot, otherwise it was part of a directory name.
        if (const size_t slash = path.find_last_of("/\\");
            slash != std::string_view::npos && slash > dot)
            return DEFAULT_MIME;

        std::string ext;
        ext.reserve(path.size() - dot - 1);
        for (const char c : path.substr(dot + 1))
            ext.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

        const auto& types = detail::table();
        const auto it = types.find(ext);
        return it != types.end() ? it->second : DEFAULT_MIME;
    }

    [[nodiscard]] inline bool is_textual(std::string_view mime_type) {
        return mime_type.starts_with("text/")
            || mime_type == "application/json"
            || mime_type == "application/javascript"
            || mime_type == "application/xml";
    }

} // namespace vibe::mime

#endif // VIBE_MIME_TYPES_HPP
