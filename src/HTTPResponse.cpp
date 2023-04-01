#include <HTTPResponse.hpp>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <utilities.hpp>
HTTP::HTTPResponse *HTTP::defaultPage(HTTP::HTTPRequest *request) {
    request->path += "index.html";

    return defaultFileFounder(request);
}

HTTP::HTTPResponse *HTTP::defaultFileFounder(HTTPRequest *request) {
    HTTP::HTTPResponseFile *file = new HTTP::HTTPResponseFile;
    struct stat buffer;
    // ignore the starting '/'
    file->fd = open(request->path.data() + 1, O_RDONLY);
    if (file->fd < 0) {
        delete file;
        return notFoundHandler(request);
    } else {
        fstat(file->fd, &buffer);
        file->file_size = buffer.st_size;
        file->headers.insert(
            {"Content-Length", std::to_string(file->file_size)});
        auto type_i = HTTP::g_mime_type_map.find(
            utility::getFileExt(request->path.data()));
        if (type_i != HTTP::g_mime_type_map.end()) {
            file->headers.insert({"Content-Type", type_i->second});
        }
    }

    return file;
}

HTTP::HTTPResponse *HTTP::notFoundHandler(HTTPRequest *request) {
    HTTP::HTTPResponseText *not_found = new HTTP::HTTPResponseText;
    not_found->status = "404 Not Found";
    not_found->body = "cao ni ma hty";
    not_found->headers.insert(
        {"Content-Length", std::to_string(not_found->body.length())});
    not_found->type = HTTP::fail;
    return not_found;
}