#include "HTTPResponse.hpp"
#include <HTTPUnit.hpp>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sys/stat.h>
#include <utilities.hpp>
using namespace std;
HTTPUnit::HTTPUnit() {
    url_map_.insert({"/", &HTTP::defaultPage});
    url_map_.insert({"/favicon.ico", &HTTP::favIcon});
    url_map_.insert({"_default_file_", &HTTP::defaultFileFounder});
}

pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
HTTPUnit::parseRequest(string &raw_request) {
    char *body_start;
    HTTP::HTTPRequest *request = new HTTP::HTTPRequest;
    try {
        body_start = parseHeader(raw_request, request->headers);
    } catch (const runtime_error &e) {
        // raw_request += "HTTP/1.1 304 OK\n\nFail";
        cout << e.what() << endl;
        return {request, new HTTP::HTTPResponse};
    }
    (void)body_start; // use body

    string &path = request->headers["path"];
    auto api_iter = url_map_.find(path);
    request->path = path.data();
    struct stat buffer;
    if (stat(request->path + 1, &buffer) == 0) {
        auto func = url_map_.at("_default_file_");
        auto response = func(request);
        raw_request += dispatchResponseHeaders(response);
        HTTP::ResponseType type = response->type;
        return {request, response};
    }
    if (api_iter != url_map_.end()) {
        auto func = api_iter->second;
        auto response = func(request);
        raw_request += dispatchResponseHeaders(response);
        HTTP::ResponseType type = response->type;
        return {request, response};
    } else {
        throw runtime_error("to be implement");
    }
}

char *HTTPUnit::parseHeader(string &raw_request, map<string, string> &headers) {
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
// HTTP::HTTPResponse *HTTPUnit::parseUrl(const string &url) {}
