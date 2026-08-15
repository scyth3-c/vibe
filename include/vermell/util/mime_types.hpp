//
// MIME type registry: maps file extensions to Content-Type values.
//

#ifndef VERMELL_MIME_TYPES_HPP
#define VERMELL_MIME_TYPES_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace vermell::mime {

    inline constexpr std::string_view DEFAULT_MIME = "application/octet-stream";

    // Common types are also available for responses that do not come from a file.
    inline constexpr std::string_view html       = "text/html";
    inline constexpr std::string_view css        = "text/css";
    inline constexpr std::string_view plain      = "text/plain";
    inline constexpr std::string_view markdown   = "text/markdown";
    inline constexpr std::string_view xml        = "application/xml";
    inline constexpr std::string_view json       = "application/json";
    inline constexpr std::string_view javascript = "application/javascript";
    inline constexpr std::string_view pdf        = "application/pdf";
    inline constexpr std::string_view binary     = DEFAULT_MIME;
    inline constexpr std::string_view png        = "image/png";
    inline constexpr std::string_view jpeg       = "image/jpeg";
    inline constexpr std::string_view svg        = "image/svg+xml";

    namespace detail {
        inline const std::unordered_map<std::string_view, std::string_view>& table() {
            static const std::unordered_map<std::string_view, std::string_view> TABLE = {
                // text
                {"html",  "text/html"},
                {"htm",   "text/html"},
                {"css",   "text/css"},
                {"csv",   "text/csv"},
                {"tsv",   "text/tab-separated-values"},
                {"txt",   "text/plain"},
                {"md",    "text/markdown"},
                {"markdown", "text/markdown"},
                {"rtf",   "text/rtf"},
                {"vtt",   "text/vtt"},
                {"yaml",  "application/yaml"},
                {"yml",   "application/yaml"},
                {"xml",   "application/xml"},
                // application
                {"json",  "application/json"},
                {"map",   "application/json"},
                {"jsonld", "application/ld+json"},
                {"ndjson", "application/x-ndjson"},
                {"js",    "application/javascript"},
                {"mjs",   "application/javascript"},
                {"cjs",   "application/javascript"},
                {"pdf",   "application/pdf"},
                {"epub",  "application/epub+zip"},
                {"zip",   "application/zip"},
                {"gz",    "application/gzip"},
                {"bz2",   "application/x-bzip2"},
                {"zst",   "application/zstd"},
                {"tar",   "application/x-tar"},
                {"rar",   "application/vnd.rar"},
                {"7z",    "application/x-7z-compressed"},
                {"wasm",  "application/wasm"},
                {"bin",   "application/octet-stream"},
                {"sql",   "application/sql"},
                {"graphql", "application/graphql"},
                {"jar",   "application/java-archive"},
                {"apk",   "application/vnd.android.package-archive"},
                {"doc",   "application/msword"},
                {"docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
                {"docm",  "application/vnd.ms-word.document.macroEnabled.12"},
                {"xls",   "application/vnd.ms-excel"},
                {"xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
                {"xlsm",  "application/vnd.ms-excel.sheet.macroEnabled.12"},
                {"ppt",   "application/vnd.ms-powerpoint"},
                {"pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
                {"pptm",  "application/vnd.ms-powerpoint.presentation.macroEnabled.12"},
                {"odt",   "application/vnd.oasis.opendocument.text"},
                {"ods",   "application/vnd.oasis.opendocument.spreadsheet"},
                {"odp",   "application/vnd.oasis.opendocument.presentation"},
                // images
                {"png",   "image/png"},
                {"jpg",   "image/jpeg"},
                {"jpeg",  "image/jpeg"},
                {"gif",   "image/gif"},
                {"apng",  "image/apng"},
                {"bmp",   "image/bmp"},
                {"svg",   "image/svg+xml"},
                {"ico",   "image/x-icon"},
                {"webp",  "image/webp"},
                {"avif",  "image/avif"},
                {"heic",  "image/heic"},
                {"heif",  "image/heif"},
                {"jxl",   "image/jxl"},
                {"tif",   "image/tiff"},
                {"tiff",  "image/tiff"},
                // audio / video
                {"mp3",   "audio/mpeg"},
                {"aac",   "audio/aac"},
                {"flac",  "audio/flac"},
                {"m4a",   "audio/mp4"},
                {"mid",   "audio/midi"},
                {"midi",  "audio/midi"},
                {"wav",   "audio/wav"},
                {"ogg",   "audio/ogg"},
                {"opus",  "audio/opus"},
                {"mp4",   "video/mp4"},
                {"m4v",   "video/mp4"},
                {"webm",  "video/webm"},
                {"ogv",   "video/ogg"},
                {"avi",   "video/x-msvideo"},
                {"mkv",   "video/x-matroska"},
                {"3gp",   "video/3gpp"},
                {"ts",    "video/mp2t"},
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
                {"sh",    "application/octet-stream"},
                {"php",   "application/octet-stream"},
                {"ps",    "application/postscript"},
                {"eps",   "application/postscript"},
            };
            return TABLE;
        }
    } // namespace detail

    // Returns the MIME type for a path or bare extension ("png", "a/b/pic.jpg").
    // Unknown or missing extensions resolve to application/octet-stream.
    [[nodiscard]] inline std::string_view of(std::string_view path) {
        // Ignore URL-style suffixes when a route passes a path with a query.
        const size_t query = path.find_first_of("?#");
        const std::string_view filename = path.substr(0, query);
        const size_t dot = filename.find_last_of('.');
        if (dot == std::string_view::npos || dot + 1 >= filename.size())
            return DEFAULT_MIME;

        // No slash may follow the dot, otherwise it was part of a directory name.
        if (const size_t slash = filename.find_last_of("/\\");
            slash != std::string_view::npos && slash > dot)
            return DEFAULT_MIME;

        std::string ext;
        ext.reserve(filename.size() - dot - 1);
        for (const char c : filename.substr(dot + 1))
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

} // namespace vermell::mime

#endif // VERMELL_MIME_TYPES_HPP
