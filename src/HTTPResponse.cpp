#include <HTTPResponse.hpp>
#include <string>
#include <sys/stat.h>
HTTP::HTTPResponse *HTTP::defaultPage(HTTP::HTTPRequest *request) {
    HTTP::HTTPResponseFile *index = new HTTP::HTTPResponseFile;
    struct stat buffer;
    index->fd = open("index.html", O_RDONLY);
    fstat(index->fd, &buffer);
    index->file_size = buffer.st_size;
    index->headers.insert({"Content-Length", std::to_string(index->file_size)});
    return index;
}

HTTP::HTTPResponse *HTTP::favIcon(HTTPRequest *request) {
    HTTP::HTTPResponseFile *icon = new HTTP::HTTPResponseFile;
    struct stat buffer;
    icon->fd = open("favicon.png", O_RDONLY);
    fstat(icon->fd, &buffer);
    icon->file_size = buffer.st_size;
    icon->headers.insert({"Content-Length", std::to_string(icon->file_size)});
    icon->headers.at("Content-Type") = "image/png";
    return icon;
}

HTTP::HTTPResponse *HTTP::defaultFileFounder(HTTPRequest *request) {
    HTTP::HTTPResponseFile *icon = new HTTP::HTTPResponseFile;
    struct stat buffer;
    // ignore the starting '/'
    icon->fd = open(request->path + 1, O_RDONLY);
    if (icon->fd < 0) {
        icon->headers["status"] = "404 Not Found";
    } else {
        fstat(icon->fd, &buffer);
        icon->file_size = buffer.st_size;
        icon->headers.insert(
            {"Content-Length", std::to_string(icon->file_size)});
        icon->headers.at("Content-Type") = " application/javascript";
    }

    return icon;
}