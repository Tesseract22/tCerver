#pragma once
#include <fcntl.h>
#include <map>
#include <string>
#include <string_view>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
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
    virtual int sendData(int socket_fd) = 0;
    virtual ~HTTPResponse() {}
};
class HTTPRequest {
  public:
    HTTPRequest() = default;
    HTTPRequest(char *path_arg,
                std::map<std::string_view, std::string_view> header_arg,
                std::map<std::string_view, std::string_view> args_arg,
                char *body_arg)
        : path(path_arg), headers(header_arg), args(args_arg), body(body_arg) {}
    std::string path;
    std::map<std::string_view, std::string_view> headers;
    std::map<std::string_view, std::string_view> args;
    const char *body;
};

class HTTPResponseText : public HTTPResponse {
  public:
    HTTPResponseText() : HTTPResponse() {
        type = text;
        headers.insert({"Content-Type", "text/plain"});
    }
    std::string body;
    int sendData(int socket_fd);
};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() { type = file; }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    int sendData(int socket_fd);
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
HTTPResponse *notFoundHandler(HTTPRequest *request);

extern const std::unordered_map<std::string, std::string> g_mime_type_map;
} // namespace HTTP
