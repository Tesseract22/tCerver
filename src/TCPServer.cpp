#include "TCPServer.hpp"
#include "HTTPResponse.hpp"
#include "HTTPUnit.hpp"
#include "MultiThreadQueue.hpp"
#include "utilities.hpp"
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>
using namespace std;

std::vector<TCPServer *> TCPServer::servers_;
TCPServer::~TCPServer() {

    for (size_t i = 0; i < listen_threads_.size(); ++i)
        listen_threads_[i].join();
    for (size_t i = 0; i < parse_threads_.size(); ++i)
        parse_threads_[i].join();

    for (auto &p : mutexes_) {
        if (p)
            delete p;
    }
}

TCPServer::TCPServer(HTTPUnit &&http, size_t listen_threads,
                     size_t parse_threads, ostream &log_io, ostream &err_io)

    : log_io_(log_io),
      err_io_(err_io),
      socket_fd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(DEFAULT_PORT),
      task_q_(),
      listen_threads_(listen_threads),
      parse_threads_(parse_threads),
      mutexes_(),
      http_(http) {
    // setting up the socket
    if (listen_threads < 1)
        throw runtime_error("threads number must be at least 1");
    epolls_.reserve(listen_threads);
    for (size_t i = 0; i < listen_threads; ++i)
        epolls_.emplace_back(EPoll(&epoll_m_, socket_fd_, &sockets_, &task_q_));
    // for (auto &e : epolls_)
    //     cout << e.epoll_fd_ << endl;
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

    chdir("../resource");
    getcwd(resource_path_, 100);
    pipe(stop_pipe_);
    servers_.push_back(this);
}

void TCPServer::sigintHandler(int dummpy) {
    char temp[4] = "end";
    for (auto server : servers_) {
        server->running_ = false;
        for (auto &e : server->epolls_) {
            e.stop();
            write(server->stop_pipe_[1], temp, 4);
        }

        for (size_t i = 0; i < server->parse_threads_.size(); ++i) {
            server->task_q_.push(NULL);
        }
    }
}

void TCPServer::waitListen(size_t id) {

    try {
        epolls_[id].wait(stop_pipe_[0]);
    } catch (SocketException &e) {
        err_io_ << e.what() << endl;
        waitListen(id);
    }
}

void TCPServer::serverStart() {

    // socket_thread_ = thread(&SocketBase::startListen, master_socket_);
    running_ = true;
    for (size_t i = 0; i < listen_threads_.size(); ++i) {
        listen_threads_[i] = thread(&TCPServer::waitListen, this, i);
    }
    for (size_t i = 0; i < parse_threads_.size(); ++i) {
        parse_threads_[i] = thread(&TCPServer::waitParse, this, i);
    }

    cout << "server started" << endl;
}

void TCPServer::serverStop() { running_ = false; }

void TCPServer::waitParse(size_t id) {
#if DEBUG
    cout << this_thread::get_id() << " start fetching task..." << endl;
#endif
    while (true) {
        Task *t = task_q_.pull();
        if (t == NULL) {
            return;
        }

        int socket_fd = t->socket_fd;
        auto result = http_.parseRequest(t->task);
        HTTP::HTTPRequest *request = result.first;
        HTTP::HTTPResponse *response = result.second;
        response->headers.insert({"Server", "tcerver"}); // server information

        string raw_headers = http_.dispatchResponseHeaders(response);
        log_io_ << "send response with headers: \n" << raw_headers << endl;
        int bytes = 0;
        size_t total_bytes = 0;
        while (total_bytes < raw_headers.length()) {
            if ((bytes = send(socket_fd, raw_headers.data(),
                              raw_headers.length(), 0)) <= 0)
                break;
            total_bytes += bytes;
        }
        response->sendData(socket_fd);
        // logRequest(request, response);
        delete request;
        delete response;
        delete t;
    }
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