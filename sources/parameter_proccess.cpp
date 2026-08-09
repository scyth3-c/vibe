#include "../include/vibe/util/parameter_proccess.h"

HTTP_QUERY::HTTP_QUERY() = default;
HTTP_QUERY::~HTTP_QUERY() = default;

string HTTP_QUERY::selectPerType(const string &target, const string &conten_type, bool &init) {
    if (target.empty())
        init = false;
    return conten_type == X_WWW_FORM ? x_www_form_urlencoded(target) : raw_form_encoded(target);
}

string HTTP_QUERY::route_refactor_params(const string& _target)  {

    const auto msg = vibe::http::Message::parse(_target);
    if (!msg)
        return NOT_PARAMS;

    const std::string_view content_type = msg->content_type();
    if (content_type.empty())
        return NOT_PARAMS;

    // urlencoded and multipart bodies are key/value carriers: hand over the
    // raw body; every other type follows the legacy "data=<body>" contract.
    if (content_type == vibe::http::TYPE_FORM_URLENCODED
        || content_type == vibe::http::TYPE_MULTIPART) {
        return msg->body.empty() ? NOT_PARAMS : msg->body;
    }

    if (msg->body.empty())
        return RAW_ERROR;

    return string(RAW_TARGET) + msg->body;
}

string HTTP_QUERY::route_refactor_params_get(const string& rawresponse) {
    const auto msg = vibe::http::Message::parse(rawresponse);
    if (!msg || msg->query.empty())
        return NOT_PARAMS;
    return msg->query;
}


string HTTP_QUERY::headers_from(const string& response)  {
    // Header block: everything between the end of the request line and the
    // blank line that separates headers from the body.
    const size_t first_eol = response.find('\n');
    if (first_eol == string::npos)
        return {};

    size_t block_end = response.find("\r\n\r\n", first_eol);
    if (block_end == string::npos)
        block_end = response.find("\n\n", first_eol);
    if (block_end == string::npos)
        block_end = response.size();

    return response.substr(first_eol, block_end - first_eol);
}

std::pair<string, string> HTTP_QUERY::route_refactor(const string& target){
    const auto msg = vibe::http::Message::parse(target);
    if (!msg)
        return {};
    return {msg->method, msg->path};
}

string HTTP_QUERY::x_www_form_urlencoded(const string &target){
    const auto msg = vibe::http::Message::parse(target);
    return msg ? msg->body : string{};
}

string HTTP_QUERY::raw_form_encoded(const string &target) {
    const auto msg = vibe::http::Message::parse(target);
    if (!msg || msg->body.empty())
        return RAW_ERROR;
    return string(RAW_TARGET) + msg->body;
}


string HTTP_QUERY::findContenType(const string &text) {
    if (text.empty())
        return STR_ERR;

    const auto msg = vibe::http::Message::parse(text);
    if (!msg)
        return STR_ERR;

    const std::string_view content_type = msg->header("Content-Type");
    if (content_type.empty())
        return STR_ERR;

    return trim(string(content_type));
}


string HTTP_QUERY::get_params(const string &target){
    const size_t start = target.find('?');
    if (start == string::npos)
        return NOT_PARAMS;

    const size_t end = target.find(' ', start);
    const string params = target.substr(start + 1, end == string::npos ? end : end - start - 1);
    if (params.empty())
        return NOT_PARAMS;
    return params;
}

string HTTP_QUERY::trim(string target){
    target.erase(std::remove_if(target.begin(), target.end(),
                                [](const unsigned char c) { return std::isspace(c); }),
                 target.end());
    return target;
}
