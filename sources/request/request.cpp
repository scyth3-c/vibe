#include "../../include/vermell/request/request.hpp"

Request::Request() = default;


void Request::consume(const vermell::http::Message& msg) {
    clear_parameters();
    _headers      = msg.headers;
    _body         = msg.body;
    _content_type = std::string(msg.content_type());
    _files.clear();

    using namespace vermell::http;

    // GET carries its parameters in the query string (legacy behavior).
    if (msg.method == "GET") {
        if (!msg.query.empty())
            _parameters = parse_query(msg.query);
        return;
    }

    const std::string_view type = msg.content_type();
    if (type.empty())
        return; // no Content-Type: no parameters (legacy NOT_PARAMS behavior)

    if (type == TYPE_FORM_URLENCODED) {
        if (!msg.body.empty())
            _parameters = parse_query(msg.body);
    }
    else if (type == TYPE_MULTIPART) {
        if (const auto boundary = msg.content_type_param("boundary"); boundary && !msg.body.empty()) {
            auto [fields, files] = parse_multipart(msg.body, *boundary);
            _parameters = std::move(fields);
            _files      = std::move(files);
        }
    }
    else {
        // application/json, text/plain and any other type: exposed as raw
        // body through raw(), and as the legacy ("data", body) parameter.
        _parameters.emplace_back("data", msg.body.empty() ? string("null") : msg.body);
    }
}


void Request::setParameters(vector<std::pair<string, string>> params) {
    _parameters = std::move(params);
}

void Request::setHeaders(vector<std::pair<string, string>> headers) {
    _headers = std::move(headers);
}

void Request::setBody(string body) {
    _body = std::move(body);
}

void Request::setContentType(string content_type) {
    _content_type = std::move(content_type);
}

void Request::setFiles(vector<vermell::http::UploadedFile> files) {
    _files = std::move(files);
}


void Request::setRawParametersData(string &&_raw) {

    if (_raw.empty() || _raw == NOT_PARAMS)
        return;

    auto parsed = vermell::http::parse_query(_raw);
    _parameters.insert(_parameters.end(),
                       std::make_move_iterator(parsed.begin()),
                       std::make_move_iterator(parsed.end()));
}


void Request::setRawHeadersData(string &&_raw){

    size_t pos = 0;
    while (pos < _raw.size()) {
        const size_t eol = _raw.find('\n', pos);
        std::string_view line = std::string_view(_raw).substr(
            pos, eol == string::npos ? eol : eol - pos);

        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        if (const size_t colon = line.find(':');
            !line.empty() && colon != std::string_view::npos && colon > 0) {
            std::string_view value = line.substr(colon + 1);
            if (!value.empty() && value.front() == ' ')
                value.remove_prefix(1);
            _headers.emplace_back(line.substr(0, colon), value);
        }

        if (eol == string::npos)
            break;
        pos = eol + 1;
    }
}


Param_t Request::getHeaders() {
    Param_t nuevo;
    try {
        nuevo.setContent(_headers);
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    return nuevo;
}

Param_t Request::getParameters() {
    Param_t nuevo;
    try {
        nuevo.setContent(_parameters);
    }
    catch(const std::exception& e) {
      std::cerr << e.what() << '\n';
    }
    return nuevo;
}
