#pragma once
#include <stdexcept>
class SocketException : std::exception {

  public:
    explicit SocketException();
    SocketException(const char *msg) : msg_(msg) {}
    SocketException(const std::string &msg) : msg_(msg) {}
    virtual ~SocketException() noexcept {}
    virtual const char *what() const noexcept { return msg_.data(); }

  protected:
    std::string msg_;
};