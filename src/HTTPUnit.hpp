#pragma once
#include <HTTPResponse.hpp>
#include <functional>
#include <map>
#include <string>
class HTTPUnit {
  public:
    HTTPUnit();

  private:
    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
};