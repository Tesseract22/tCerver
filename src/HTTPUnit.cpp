#include "HTTPResponse.hpp"
#include <HTTPUnit.hpp>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utilities.hpp>
using namespace std;
HTTPUnit::HTTPUnit() { url_map_.insert({"/", &HTTP::defaultPage}); }

char *HTTPUnit::parseRequest(string &raw_request, char *response,
                             map<string, string> &headers) {
    char *body_start;
    try {
        body_start = parseHeader(raw_request, headers);
    } catch (runtime_error) {
        return NULL;
    }
    string &path = headers["path"];
    auto api_iter = url_map_.find(path);
    HTTP::HTTPRequest request;
    request.path = path.data();
    request.headers = std::move(headers);
    if (api_iter != url_map_.end()) {
        auto func = api_iter->second;
        auto response = func(&request);
    }

    return body_start;
}

char *HTTPUnit::parseHeader(string &raw_request, map<string, string> &headers) {
    cout << "request: \n" << raw_request << endl;
    char *c_str = raw_request.data();
    int idx;
    // method
    if ((idx = utility::incrementParse(c_str, " ") < 0))
        throw runtime_error("header has incorrect format");

    c_str[idx] = '\0';
    headers.insert({"method", c_str});
    c_str += idx + 1;
    // path
    if ((idx = utility::incrementParse(c_str, " ") < 0))
        throw runtime_error("header has incorrect format");
    c_str[idx] = '\0';
    headers.insert({"path", c_str});
    c_str += idx + 1;
    // http version, we support HTTP 1.1
    if ((idx = utility::incrementParse(c_str, "\n") < 0))
        throw runtime_error("header has incorrect format");
    c_str[idx] = '\0';
    headers.insert({"http_var", c_str});
    c_str += idx + 1;

    // parse the rest of the header

    // cout << headers["path"] << endl;
    cout << "rset of header: " << '\n';
    while (idx != -1) { // err code
                        // seperate each line

        if ((idx = utility::incrementParse(c_str, "\n") < 0))
            throw runtime_error("header has incorrect format");
        int next_line_idx = idx;
        char *start_line_ptr = c_str;
        c_str[idx] = '\0';
        if (strcmp(c_str, "\r") == 0) {
            // end of header
            // see
            // https://stackoverflow.com/questions/5757290/http-header-line-break-style
            break;
        }
        // and then seperate by ": "
        if ((idx = utility::incrementParse(c_str, ": ") < 0))
            throw runtime_error("header has incorrect format");
        c_str[idx] = '\0';
        char *key = c_str;
        c_str += idx + 2; // extra = strlen(sep)
        char *val = c_str;
        headers.insert({key, val});
        c_str = start_line_ptr + next_line_idx + 1;
    }
    return c_str;
}

// HTTP::HTTPResponse *HTTPUnit::parseUrl(const string &url) {}
