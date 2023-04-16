#include "TCPServer.hpp"
#include "HTTPResponse.hpp"
#include "HTTPUnit.hpp"
#include "Logs.hpp"
#include "MultiThreadQueue.hpp"
#include "Scheduler.hpp"
#include "Task.hpp"
#include "utilities.hpp"
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <utility>
#include <vector>
using namespace std;

std::vector<TCPServer *> TCPServer::servers_;
TCPServer::~TCPServer() {}

TCPServer::TCPServer(HTTPUnit &&http, size_t listen_threads, ostream &log_io,
                     ostream &err_io)

    : log_io_(log_io),
      err_io_(err_io),
      socket_fd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(DEFAULT_PORT),
      listen_threads_(listen_threads),
      mutexes_(),
      http_(http),
      s_(10) {
    // setting up the socket
    if (listen_threads < 1)
        throw runtime_error("threads number must be at least 1");
    epolls_.reserve(listen_threads);
    for (size_t i = 0; i < listen_threads; ++i)
        epolls_.emplace_back(
            EPoll(epoll_m_, socket_fd_, sockets_, mutexes_, s_,
                  [this](string &str) { return handleRequest(str); }));
    addr_.sin_family = AF_INET;

    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port_);
    int on = 1;
    // non blocking and reuseable
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    fcntl(socket_fd_, F_SETFL, O_NONBLOCK);

    if (bind(socket_fd_, (sockaddr *)&addr_, sizeof(addr_)) < 0) {
        throw runtime_error("bind failed");
    }

    if (listen(socket_fd_, 3) < 0)
        throw runtime_error("listen failed");

    if (chdir("../resource"))
        throw std::runtime_error("connot open directory ../resource");
    getcwd(resource_path_, 100);
    pipe(stop_pipe_);
    // servers_.push_back(this);

    // std::signal(SIGINT, TCPServer::SIGINT_HANDLER);
}

void TCPServer::SIGINT_HANDLER(int dummpy) {

    for (auto s : servers_) {
        s->running_ = false;
        for (auto &e : s->epolls_) {
            e.stop();
        }
        for (auto &e : s->epolls_) {
            cerr << "write\n";
            write(s->stop_pipe_[1], "0000", 5);
        }
        close(s->stop_pipe_[0]);
        close(s->stop_pipe_[1]);
    }
    Scheduler::SIGINT_HANDLER(dummpy);
}

void TCPServer::waitListen(size_t id) {

    for (size_t i = 0; i < listen_threads_; ++i) {
        epolls_[id].wait(stop_pipe_[0]); // start coroutine;
    }
}

void TCPServer::serverStart() {

    running_ = true;
    for (size_t i = 0; i < epolls_.size(); ++i) {
        waitListen(i);
    }
}

void TCPServer::serverStop() { running_ = false; }

Task<TCPServer::Response> TCPServer::handleRequest(std::string &str) {
    co_await s_;
    auto result = http_.parseRequest(str);
    HTTP::HTTPRequest *request = result.first;
    HTTP::HTTPResponse *response = result.second;
    response->headers.insert({"Server", "tcerver"}); // server
    TCPServer::Response tcp_response;
    tcp_response.str = http_.getResponseHeaders(response);
    if (response->getBody().index() == 0) {
        tcp_response.fd = std::get<int>(response->getBody());
    } else {
        tcp_response.str += std::move(std::get<string>(response->getBody()));
    }
    // logRequest(request, response);
    delete request;
    delete response;
    co_return tcp_response;
}

void TCPServer::logRequest(HTTP::HTTPRequest *request,
                           HTTP::HTTPResponse *response) {
    log_io_ << request->path << ' ';
    log_io_ << request->headers["method"] << ' ';
    log_io_ << response->status << ' ';
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    log_io_ << ctime(&now);
    log_io_ << endl;
}