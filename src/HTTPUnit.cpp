#include "HTTPUnit.hpp"
#include "HTTPResponse.hpp"
#include "utilities.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
using namespace std;
HTTPUnit::HTTPUnit() {
    url_map_.insert({"/", &HTTP::defaultPage});
    url_map_.insert({"_default_file_", &HTTP::defaultFileFounder});
}

pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
HTTPUnit::parseRequest(string &raw_request) {
    char *body_start;
    HTTP::HTTPRequest *request = new HTTP::HTTPRequest;
    try {
        body_start = parseHeader(raw_request, request->headers);
    } catch (const runtime_error &e) {
        return {request, HTTP::notFoundHandler(request)};
    }
    (void)body_start; // use body

    string_view path = request->headers["path"];
    request->path = path.data();
    struct stat buffer;
    auto api_iter = url_map_.find(path.data());
    HTTP::HTTPResponse *response;
    if (api_iter != url_map_.end()) { // url map has highest priorities
        auto &func = api_iter->second;
        response = func(request);

    } else if (stat(request->path.data() + 1, &buffer) == 0) {
        auto &func = url_map_.at("_default_file_");
        response = func(request);

    } else {
        response = HTTP::notFoundHandler(request);
    }
    return {request, response};
}
char *HTTPUnit::parseHeader(string &raw_request,
                            map<string_view, string_view> &headers) {
    char *c_str = raw_request.data();
    int idx;
    // method
    if ((idx = utility::incrementParse(c_str, " ")) < 0)
        throw runtime_error("header has incorrect format, method");
    c_str[idx] = '\0';
    headers.insert({"method", c_str});
    c_str += idx + 1;
    // path
    if ((idx = utility::incrementParse(c_str, " ")) < 0)
        throw runtime_error("header has incorrect format, path");
    c_str[idx] = '\0';
    headers.insert({"path", c_str});
    c_str += idx + 1;
    // http version, we support HTTP 1.1
    if ((idx = utility::incrementParse(c_str, "\n")) < 0)
        throw runtime_error("header has incorrect format, http version");
    c_str[idx] = '\0';
    headers.insert({"http_var", c_str});
    c_str += idx + 1;

    // parse the rest of the header
    while (idx != -1) { // err code
        // seperate each line

        if ((idx = utility::incrementParse(c_str, "\n")) < 0)
            throw runtime_error("header has incorrect format, header key");
        int next_line_idx = idx;
        char *start_line_ptr = c_str;
        c_str[idx] = '\0';
        if (strcmp(c_str, "\r") == 0) {
            // end of header
            // see
            //
            // https://stackoverflow.com/questions/5757290/http-header-line-break-style
            break;
        }
        // and then seperate by ": "
        if ((idx = utility::incrementParse(c_str, ": ")) < 0)
            throw runtime_error("header has incorrect format, header value");
        c_str[idx] = '\0';
        char *key = c_str;
        c_str += idx + 2; // extra = strlen(sep)
        char *val = c_str;
        headers.insert({key, val});
        c_str = start_line_ptr + next_line_idx + 1;
    }
    return c_str;
}
string HTTPUnit::dispatchResponseHeaders(HTTP::HTTPResponse *response) {
    string res;
    res.reserve(100);
    //          "HTTP/1.1 200 OK\nContent-Type: "
    //          "text/plain\nContent-Length: "
    //          "12\n\n",
    res += response->http_ver;
    res += " ";
    res += response->status;
    res += '\n';
    for (auto iter = response->headers.begin(); iter != response->headers.end();
         ++iter) {
        res += iter->first;
        res += ": ";
        res += iter->second;
        res += '\n';
    }
    res += '\n';
    return res;
}

void HTTPUnit::bindUrl(
    const std::string &url,
    const std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)> &func) {

    url_map_.insert({url, func});
}
