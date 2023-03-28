#pragma once
#include <HTTPResponse.hpp>
#include <functional>
#include <map>
#include <string>
class HTTPUnit {
  public:
    HTTPUnit();

    char *parseRequest(std::string &raw_request, char *response,
                       std::map<std::string, std::string> &headers);
    HTTP::HTTPResponse *parseUrl(const std::string &url);

  private:
    char *parseHeader(std::string &raw_request,
                      std::map<std::string, std::string> &headers);

    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
};