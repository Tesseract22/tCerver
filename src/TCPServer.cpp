#include "EPoll.hpp"
#include "HTTPResponse.hpp"
#include "MultiThreadQueue.hpp"
#include <HTTPUnit.hpp>
#include <TCPServer.hpp>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utilities.hpp>
#include <vector>
using namespace std;

// TCPServer::TCPServer(ostream &log, ostream &err) {
//     log_io_ = &log;
//     err_io_ = &err;
//     master_socket_ = new Socket();
//     running_ = false;
//     chdir("../resource");
//     getcwd(resource_path_, sizeof(resource_path_));
//     cout << resource_path_ << endl;
// }

TCPServer::~TCPServer() {
    for (size_t i = 0; i < listen_threads_.size(); ++i)
        listen_threads_[i].join();
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
      epolls_(listen_threads_.size(),
              EPoll(&epoll_m_, socket_fd_, &sockets_, &mutexes_, &task_q_)),
      http_(http) {
    // setting up the socket
    if (listen_threads < 1)
        throw runtime_error("threads number must be at least 1");
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
}

void TCPServer::waitListen(size_t id) { epolls_[id].wait(); }

void TCPServer::serverStart() {

    // socket_thread_ = thread(&SocketBase::startListen, master_socket_);
    for (size_t i = 0; i < listen_threads_.size(); ++i) {
        listen_threads_[i] = thread(&TCPServer::waitListen, this, i);
    }
    for (size_t i = 0; i < parse_threads_.size(); ++i) {
        parse_threads_[i] = thread(&TCPServer::waitParse, this, i);
    }
    running_ = true;
}

void TCPServer::waitParse(size_t id) {
#if DEBUG
    cout << this_thread::get_id() << " start fetching task..." << endl;
#endif
    while (running_) {
        Task *t = task_q_.pull();
        int socket_fd = t->socket_fd;

        auto result = http_.parseRequest(t->task);
        HTTP::HTTPRequest *request = result.first;
        HTTP::HTTPResponse *response = result.second;
        response->headers.insert({"Server", "tcever"}); // server information

        string raw_response = http_.dispatchResponseHeaders(response);
        int bytes =
            send(socket_fd, raw_response.data(), raw_response.length(), 0);
        if (response->type == HTTP::fail) {
            // ???
        } else if (response->type == HTTP::file) {
            auto *file_response = (HTTP::HTTPResponseFile *)response;
            if (file_response->fd > 0) {
                off_t offset = 0;
                while ((bytes = sendfile(socket_fd, file_response->fd, &offset,
                                         10240)) > 0) {
                }
                close(file_response->fd);
            }
        }
        logRequest(request, response);
        (void)bytes;
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