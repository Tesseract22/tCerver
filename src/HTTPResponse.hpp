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
#include <variant>
#include <vector>

namespace HTTP {
struct HTTPBody {
    std::map<std::string, std::string> args;
    struct HTTPFile {
        std::string file_name;
        std::string name;
        std::string disposition;
        std::string_view bytes;
    };
    std::vector<HTTPFile> bytes;
};
enum ResponseType {
    fail = -1,
    text = 0,
    file = 1,
};
class HTTPResponse {
  public:
    std::map<std::string, std::string> headers;
    std::string status = "200 OK";
    std::string http_ver = "HTTP/1.1";
    ResponseType type = text;
    virtual std::variant<int, std::string> getBody() = 0;
    virtual ~HTTPResponse() {}
};
class HTTPRequest {
  public:
    HTTPRequest() = default;
    HTTPRequest(char *path_arg,
                std::map<std::string_view, std::string_view> header_arg,
                std::map<std::string, std::string> args_arg, char *body_arg)
        : path(path_arg), headers(header_arg), args(args_arg) {}
    std::string path;
    std::string method;
    std::string protocol;
    std::map<std::string_view, std::string_view> headers;
    std::map<std::string, std::string> args;
    HTTPBody body;
    std::string_view raw_body;
};

class HTTPResponseText : public HTTPResponse {
  public:
    HTTPResponseText() : HTTPResponse() {
        type = text;
        headers.insert({"Content-Type", "text/plain"});
        headers.insert({"Content-Length", "0"});
    }
    std::string body;
    std::variant<int, std::string> getBody();
};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() { type = file; }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    std::variant<int, std::string> getBody();
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
HTTPResponse *notFoundHandler(HTTPRequest *request);

extern const std::unordered_map<std::string, std::string> g_mime_type_map;
} // namespace HTTP
