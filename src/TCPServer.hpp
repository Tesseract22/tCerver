#pragma once
#include "HTTPResponse.hpp"
#include "HTTPUnit.hpp"
#include <EPoll.hpp>
#include <MultiThreadQueue.hpp>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
// TCP server implementation
#define DEFAULT_PORT 8080

class TCPServer {
  public:
    // TCPServer(std::ostream &log = std::cout, std::ostream &err = std::cerr);
    TCPServer(HTTPUnit &&http, size_t listen_threads = 2,
              size_t parse_threads = 2, std::ostream &log_io = std::cout,
              std::ostream &err_io = std::cerr);
    // TCPServer();
    ~TCPServer();
    void startListen();
    void stopListen();

    void lockSocket(int socket_fd) { epolls_[0].lockSocket(socket_fd); }
    void unlockSocket(int socket_fd) { epolls_[0].unlockSocket(socket_fd); }

    int getStatus(int socket_fd) { return sockets_[socket_fd]; }
    void modSocket(int socket_fd, int act) {
        epolls_[0].modSocket(socket_fd, act);
    }
    void delSocket(int socket_fd) { epolls_[0].delSocket(socket_fd); }

    void waitTask(size_t id);

    void logRequest(HTTP::HTTPRequest *request, HTTP::HTTPResponse *response);

  private:
    bool running_;

    std::ostream &log_io_;
    std::ostream &err_io_;

    /**
      @new
    */
    int socket_fd_;
    sockaddr_in addr_;
    int port_;

    MultiThreadQueue<Task *> task_q_;

    // An array of threads listening to requests, we are not responsible for
    // allocating and deallocating this
    std::vector<std::thread> listen_threads_;
    std::vector<std::thread> parse_threads_;
    // Any array of EPoll object. Each one correspond to one listen_threads_;

    std::vector<EPoll> epolls_;

    std::vector<int> sockets_;
    std::vector<std::mutex *> mutexes_;

    std::mutex epoll_m_;

    HTTPUnit http_;

    char resource_path_[100];
};
