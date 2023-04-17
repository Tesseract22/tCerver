#include "HTTPUnit.hpp"
#include "HTTPResponse.hpp"
#include "Logs.hpp"
#include "utilities.hpp"
#include <cstddef>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>

#define GET_SV_MATCH(str, svmatch, index)                                      \
    string_view(str.data() + svmatch.position(index),                          \
                (size_t)svmatch.length(index))

using namespace std;
typedef std::match_results<std::string_view::const_iterator> svmatch;

HTTPUnit::HTTPUnit() { url_map_.insert({"/", &HTTP::defaultPage}); }

pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
HTTPUnit::parseRequest(string &raw_request) {
    // LOG(raw_request);
    char *ptr;
    HTTP::HTTPRequest *request = new HTTP::HTTPRequest;
    try {
        ptr = dispatchHTTP(raw_request, request);
        ptr = dispatchHeader(ptr, request->headers);
        request->raw_body = ptr;
        request->args = parseArgs(request->path.data());
    } catch (const runtime_error &e) {
        return {request, HTTP::notFoundHandler(request)};
    }; // use body
#if DEBUG
    LOG(request->path, " ", request->method)
#endif
    handleMethod(request);
    struct stat stat_buf;
    auto api_iter = url_map_.find(request->path.data());
    HTTP::HTTPResponse *response;
    if (api_iter != url_map_.end()) { // url map has highest priorities
        auto &func = api_iter->second;
        response = func(request);

    } else if (stat(request->path.data() + 1, &stat_buf) == 0) {
        auto &func = HTTP::defaultFileFounder;
        response = func(request);

    } else {
        LOG(404);
        response = HTTP::notFoundHandler(request);
    }
    return {request, response};
}

void HTTPUnit::handleMethod(HTTP::HTTPRequest *request) {
    const static string POST = "POST";
    const static string GET = "GET";
    const static string POST_RAW_FORM = "application/x-www-form-urlencoded";
    const static string POST_FORM_DATA = "multipart/form-data";
    if (request->method == POST) {
        try {
            auto &content_type = request->headers.at("Content-Type");

            // cerr << "[" << request->headers.at("Content-Type") << "]   ";
            // string_view test = content_type;
            if (content_type.starts_with(POST_RAW_FORM)) {
                LOG(request->raw_body)
                request->body.args = parseArgs(request->raw_body.data());
            } else if (content_type.starts_with(POST_FORM_DATA)) {
                regex boundary_re("boundary=\"?(.+?)\"?[;|$|\n|\r]");
                // std::match_resu boundary_cm;
                svmatch boundary_cm;
                if (regex_search(content_type.cbegin(), content_type.cend(),
                                 boundary_cm, boundary_re)) {
                    string boundary = boundary_cm[1];
                    // LOG(boundary);
                    regex form_data_reg("Content-Disposition: "
                                        "(.+?)[\n|\r]{2}([\\s\\S]+?)--" +
                                        boundary);
                    svmatch form_data_svm;
                    // const char *form = request->raw_body.data();
                    string_view form = request->raw_body;
                    while (regex_search(form.begin(), form.end(), form_data_svm,
                                        form_data_reg)) {
                        request->body.bytes.push_back(parseFormDataInfo(
                            GET_SV_MATCH(request->raw_body, form_data_svm, 1)));
                        request->body.bytes.back().content =
                            GET_SV_MATCH(request->raw_body, form_data_svm, 2);
                        form = string_view();
                        form = form_data_svm.suffix().str();
                    }

                    // request->body.bytes.push_back({})
                }
            }
            // string &boundary = seps.at(1);
            // cout << type << ' ' << boundary << '\n';
        } catch (const std::out_of_range &e) {
            LOG("error");
        }
    } else {
    }
}

char *HTTPUnit::dispatchHTTP(std::string &raw_request,
                             HTTP::HTTPRequest *request) {
    char *c_str = raw_request.data();
    int idx;
    // method
    if ((idx = utility::incrementParse(c_str, " ")) < 0)
        throw runtime_error("header has incorrect format, method");
    c_str[idx] = '\0';
    request->method = c_str;
    c_str += idx + 1;
    // path
    if ((idx = utility::incrementParse(c_str, " ")) < 0)
        throw runtime_error("header has incorrect format, path");
    c_str[idx] = '\0';
    request->path = c_str;
    c_str += idx + 1;
    // http version, we support HTTP 1.1
    if ((idx = utility::incrementParse(c_str, "\n")) < 0)
        throw runtime_error("header has incorrect format, http version");
    c_str[idx] = '\0';
    request->protocol = c_str;
    c_str += idx + 1;
    return c_str;
}
char *HTTPUnit::dispatchHeader(char *c_str,
                               map<string_view, string_view> &headers) {
    int idx = 0;
    // method

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
            // https://stackoverflow.com/questions/5757290/http-header-line-break-style
            c_str += idx + 1;
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
string HTTPUnit::getResponseHeaders(HTTP::HTTPResponse *response) {
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

map<string, string> HTTPUnit::parseUrl(string &url) noexcept {
    char *c_str = url.data();
    int question_mark = utility::incrementParse(c_str, "?");
    if (question_mark == -1)
        return {};
    c_str += question_mark + 1;
    return parseArgs(c_str);
}
map<string, string> HTTPUnit::parseArgs(const char *c_str) noexcept {
    // alternative regex str_expr("[(\?|&)]([^=]+)=([^&#]+)");
    regex reg("([^?]*?)=(.*?)(&|$|\r|\n)");
    cmatch cm;
    map<string, string> res;
    const char *search = c_str;
    while (regex_search(search, cm, reg)) {
        res[cm[1]] = cm[2];
        search = cm.suffix().first;
    }
    return res;
}

std::vector<std::string>
HTTPUnit::parseSemiColSeperated(const string &str) noexcept {
    regex semi_col_reg("[^;]+?(?=;|$)");
    smatch sm_result;
    string search = str;
    vector<string> res;
    while (regex_search(search, sm_result, semi_col_reg)) {
        res.push_back(sm_result[0]);
        search = sm_result.suffix();
    }
    return res;
}

HTTP::HTTPBody::HTTPFile
HTTPUnit::parseFormDataInfo(const string_view &str) noexcept {
    HTTP::HTTPBody::HTTPFile info;
    regex reg("(.*?)(?:$|;)\\s?(?:name=\"(.*?)\"(?:$|;))?\\s?(?:filename=\"(.*?"
              ")\")?");
    svmatch svm;
    regex_match(str.begin(), str.end(), svm, reg);
    info.disposition = svm[1];
    if (svm.size() >= 3) {
        info.name = svm[2];
    }
    if (svm.size() >= 4) {
        info.file_name = svm[3];
    }

    return info;
}

void HTTPUnit::bindUrl(
    const string &url,
    const function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)> &func) {

    url_map_.insert({url, func});
}
