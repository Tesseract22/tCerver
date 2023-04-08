#include "HTTPResponse.hpp"
#include "utilities.hpp"
#include <iostream>
#include <string>
#include <sys/stat.h>
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
        file->headers["Content-Length"] = std::to_string(file->file_size);
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
    not_found->body = "404";
    not_found->headers["Content-Length"] =
        std::to_string(not_found->body.length());

    not_found->type = HTTP::fail;
    return not_found;
}

std::variant<int, std::string> HTTP::HTTPResponseText::getBody() {
    return body;
}

std::variant<int, std::string> HTTP::HTTPResponseFile::getBody() { return fd; }

const std::unordered_map<std::string, std::string> HTTP::g_mime_type_map = {
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