#include "EPoll.hpp"
#include "MultiThreadQueue.hpp"
#include <HTTPUnit.hpp>
#include <TCPServer.hpp>
#include <condition_variable>
#include <cstddef>
#include <fcntl.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/sendfile.h>
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

TCPServer::TCPServer(size_t listen_threads, size_t parse_threads)
    : socket_fd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(DEFAULT_PORT),
      task_q_(),
      listen_threads_(listen_threads),
      parse_threads_(parse_threads),
      epolls_(listen_threads_.size(),
              EPoll(&epoll_m_, socket_fd_, &sockets_, &mutexes_, &task_q_)) {
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

// void TCPServer::startSocket() {
//     master_socket_->lis
// }

void TCPServer::startListen() {

    // socket_thread_ = thread(&SocketBase::startListen, master_socket_);
    for (size_t i = 0; i < listen_threads_.size(); ++i) {
        listen_threads_[i] = thread(&EPoll::wait, &epolls_[i]);
    }
    for (size_t i = 0; i < parse_threads_.size(); ++i) {
        parse_threads_[i] = thread(&TCPServer::waitTask, this, i);
    }
    running_ = true;
}

void TCPServer::waitTask(size_t id) {
    cout << this_thread::get_id() << " start fetching task..." << endl;
    char buffer[1025];
    int buffer_size = 1024;
    while (running_) {
        Task *t = task_q_.pull();
        int socket_fd = t->socket_fd;
        char response[1024];
        map<string, string> headers;
        char *body_start = http_.parseRequest(t->task, response, headers);
        if (body_start == NULL) {
            continue;
        }
        string &path = headers["path"];
        try {
            // http_
        }

        delete t;
    }
}