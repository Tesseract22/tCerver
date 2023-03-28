#pragma once
#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace HTTP {
class HTTPResponse {
  public:
    std::string headers;
};

class HTTPRequest {};

class HTTPResponseText : public HTTPResponse {};

class HTTPResponseFile : public HTTPResponse {
  public:
    int fd;
    ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);

} // namespace HTTP
