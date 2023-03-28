#include <HTTPResponse.hpp>

HTTP::HTTPResponse *HTTP::defaultPage(HTTP::HTTPRequest *request) {
    HTTP::HTTPResponseFile *index = new HTTP::HTTPResponseFile;
    index->fd = open("index.html", O_RDONLY);
    return index;
}