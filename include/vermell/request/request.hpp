#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "parameters.hpp"
#include "../http/message.hpp"
#include "../util/sysprocess.h"

using std::string;
using std::vector;

constexpr const char* NOT_PARAMS = "___s";

class Request {
    string route{};

    vector<std::pair<string, string>> _parameters{};
    vector<std::pair<string, string>> _headers{};

    string _body{};          // raw request body (json, text, binary...)
    string _content_type{};  // media type without parameters, lowercased
    vector<vermell::http::UploadedFile> _files{}; // multipart/form-data uploads

public:
    Request();

    [[nodiscard]] inline size_t total_parameters() const { return _parameters.size(); }
    [[nodiscard]] inline size_t total_headers() const { return _headers.size(); }
    [[nodiscard]] Param_t getParameters();
    [[nodiscard]] Param_t getHeaders();

    inline void clear_parameters() noexcept {
        _parameters.clear();
        route.clear();
    };

    // ---- raw-body / content-type access (json, text, binary bodies) ----

    [[nodiscard]] inline const string& raw() const noexcept { return _body; }
    [[nodiscard]] inline std::string_view contentType() const noexcept { return _content_type; }
    [[nodiscard]] inline bool hasBody() const noexcept { return !_body.empty(); }

    // ---- multipart/form-data uploads ----

    [[nodiscard]] inline bool hasFiles() const noexcept { return !_files.empty(); }
    [[nodiscard]] inline size_t total_files() const noexcept { return _files.size(); }
    [[nodiscard]] inline const vector<vermell::http::UploadedFile>& files() const noexcept { return _files; }

    // First uploaded file for a given form field, nullptr when absent.
    [[nodiscard]] const vermell::http::UploadedFile* file(std::string_view field) const noexcept {
        const auto it = std::find_if(_files.begin(), _files.end(),
                                     [&](const auto& f) { return f.field == field; });
        return it != _files.end() ? &*it : nullptr;
    }

    // ---- population (used by the routing pipeline) ----

    // Fills the whole object from an already parsed HTTP message.
    void consume(const vermell::http::Message&);

    void setParameters(vector<std::pair<string, string>>);
    void setHeaders(vector<std::pair<string, string>>);
    void setBody(string);
    void setContentType(string);
    void setFiles(vector<vermell::http::UploadedFile>);

    // Legacy raw-string entry points ("a=1&b=2" / raw header blocks).
    void setRawParametersData(string &&);
    void setRawHeadersData(string &&);
};

#endif // ! REQUEST_HPP
