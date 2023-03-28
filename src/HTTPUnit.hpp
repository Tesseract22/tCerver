#pragma once
#include <HTTPResponse.hpp>
#include <functional>
#include <map>
#include <string>
class HTTPUnit {
  public:
    HTTPUnit();

    std::pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
    parseRequest(std::string &raw_request);

    std::string dispatchResponseHeaders(HTTP::HTTPResponse *response);

  private:
    char *parseHeader(std::string &raw_request,
                      std::map<std::string, std::string> &headers);

    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
};
