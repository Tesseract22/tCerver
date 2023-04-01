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
    int sendData(int socket_fd) {
        int bytes = 0;
        int total = 0;
        while ((bytes = send(socket_fd, body.data(), body.length(), 0)) > 0) {
            total += bytes;
        }
        return total;
    }
};

class HTTPResponseFile : public HTTPResponse {

  public:
    HTTPResponseFile() : HTTPResponse() { type = file; }
    std::string path_to_file;
    int fd = -1;
    size_t file_size;
    int sendData(int socket_fd) {
        off_t offset = 0;
        int bytes = 0;
        int total = 0;
        while ((bytes = (sendfile(socket_fd, fd, &offset, 10240))) > 0) {
            total += bytes;
        }
        return total;
    }
    // ~HTTPResponseFile() { close(fd); }
};

HTTPResponse *defaultPage(HTTPRequest *request);
HTTPResponse *defaultFileFounder(HTTPRequest *request);
HTTPResponse *notFoundHandler(HTTPRequest *request);

const static std::unordered_map<std::string, std::string> g_mime_type_map = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avif", "image/avif"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".cda", "application/x-cdf"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/"
              "vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gz", "application/gzip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".opus", "audio/opus"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".php", "application/x-httpd-php"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/"
              "vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/vnd.rar"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ts", "video/mp2t"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/"
              "vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"},
};
} // namespace HTTP
