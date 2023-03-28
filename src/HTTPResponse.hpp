#pragma once
#include <fcntl.h>
#include <map>
#include <string>
#include <unistd.h>

typedef std::map<std::string, std::string> s_map_t;

namespace HTTP {
class HTTPResponse {
  public:
    s_map_t headers;
    short status = 200;
    std::string http_ver = "HTTP/1.1";
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
    }
    std::string path_to_file;
    int fd = -1;
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);

} // namespace HTTP
