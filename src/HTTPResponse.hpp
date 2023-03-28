#pragma once
#include <fcntl.h>
#include <map>
#include <string>
#include <unistd.h>

typedef std::map<std::string, std::string> s_map_t;

namespace HTTP {
enum ResponseType {
    fail = -1,
    text = 0,
    file = 1,
};
class HTTPResponse {
  public:
    s_map_t headers;
    std::string status = "200 OK";
    std::string http_ver = "HTTP/1.1";
    ResponseType type = text;
    char *body;
};

class HTTPRequest {
  public:
    HTTPRequest() = default;
    HTTPRequest(char *path_arg, s_map_t header_arg, s_map_t args_arg,
                char *body_arg)
        : path(path_arg), headers(header_arg), args(args_arg), body(body_arg) {}
    char *path;
    s_map_t headers;
    s_map_t args;
    char *body;
};

class HTTPResponseText : public HTTPResponse {};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() {
        headers.insert({"Content-Type", "text/html; charset=utf-8"});
        type = file;
    }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *favIcon(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
} // namespace HTTP
