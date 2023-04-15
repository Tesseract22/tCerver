#pragma once
#include "HTTPResponse.hpp"
#include <functional>
#include <map>
#include <string>
#include <string_view>
class HTTPUnit {
  public:
    HTTPUnit();

    std::pair<HTTP::HTTPRequest *, HTTP::HTTPResponse *>
    parseRequest(std::string &raw_request);
    std::string getResponseHeaders(HTTP::HTTPResponse *response);

    void bindUrl(
        const std::string &url,
        const std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)> &func);

  private:
    void handleMethod(HTTP::HTTPRequest *request);
    char *dispatchHTTP(std::string &raw_request, HTTP::HTTPRequest *request);
    char *dispatchHeader(char *c_str,
                         std::map<std::string_view, std::string_view> &headers);
    /**
     * @brief parse the given url and return the key:val pairs contained.
     * Example: /index.html?aaa=bbb&xxx=yyy
      would return {"aaa":"bbb, "xxx":"yyy"}
     * @param url std::string&. Incorret formatted url would cause undefined
     behavior, though it would not throw
     * @return std::map<std::string, std::string> containing the args paris
     */
    std::map<std::string, std::string> parseUrl(std::string &url) noexcept;
    /**
     * @brief A helper function to parse url and
     * application/x-www-form-urlencoded.
     * See https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
     *
     * @param c_str char* c_str.
     * @return std::map<std::string, std::string> containing the args paris
     */
    std::map<std::string, std::string> parseArgs(const char *c_str) noexcept;
    std::map<std::string,
             std::function<HTTP::HTTPResponse *(HTTP::HTTPRequest *)>>
        url_map_;
    std::vector<std::string>
    parseSemiColSeperated(const std::string &str) noexcept;
    HTTP::HTTPBody::HTTPFile parseFormDataInfo(const char *str) noexcept;
};
