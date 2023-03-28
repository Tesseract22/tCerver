#include "EPoll.hpp"
#include "MultiThreadQueue.hpp"
#include <HTTPUnit.hpp>
#include <TCPServer.hpp>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstring>
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

TCPServer::TCPServer(size_t listen_threads, size_t parse_threads)
    : socket_fd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(DEFAULT_PORT),
      listen_threads_(listen_threads),
      parse_threads_(parse_threads),
      epolls_(vector<EPoll>(
          listen_threads_.size(),
          EPoll(&epoll_m_, socket_fd_, &sockets_, &mutexes_, &task_q_))) {
    // setting up the socket
    if (listen_threads <= 1)
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
    running_ = true;
}

// void TCPServer::parseRequest(char *str, int socket_fd) {
//     int idx;
//     if ((idx = utility::incrementParse(str, " ")) < 0)
//         throw runtime_error("failed to parse");
//     // [METHOD] [PATH] [REQUEST]

//     // method
//     str[idx] = '\0';
//     cout << str << endl;
//     str += idx + 1;
//     if ((idx = utility::incrementParse(str, " ")) < 0)
//         throw runtime_error("failed to parse");
//     // path
//     str[idx] = '\0';
//     cout << str << endl;
//     cout << strcmp(str, "/") << endl;
//     if (strcmp(str, "/") == 0) {

//         int fd = open("index.html", O_RDONLY);
//         cout << "opening index: " << fd << endl;
//         // fopen(const char *__restrict filename, const char *__restrict
//         modes) off_t offset = 0; int btyes; while ((btyes =
//         sendfile(socket_fd, fd, &offset, 1000)) > 0) {
//             cout << "sending file to socket: " << socket_fd
//                  << ", offset: " << offset << ", bytes sent: " << btyes <<
//                  endl;
//         }
//         cout << btyes << endl;
//         cout << "err: " << errno << endl;
//     }
//     str += idx + 1;
//     if ((idx = utility::incrementParse(str, " ")) < 0)
//         throw runtime_error("failed to parse");
//     // request
//     str[idx] = '\0';
//     cout << str << endl;
//     str += idx + 1;

//     for (int i = 0; i < 5; ++i) {
//         int idx = utility::incrementParse(str, "\n");
//         if (idx < 0)
//             break;
//         str[idx] = '\0';
//         cout << i << ": " << str << endl;
//         str += idx + 1;
//     }
// }

void TCPServer::waitTask(size_t id) {
    cout << this_thread::get_id() << " start fetching task..." << endl;
    char buffer[1025];
    int buffer_size = 1024;
    while (running_) {
        Task *t = task_q_.pull();
        int socket_fd = t->socket_fd;
        lockSocket(socket_fd);
        int status = getStatus(socket_fd);
        unlockSocket(socket_fd);
        if (status & PendingRead) {
            cout << "socket: " << socket_fd << endl;
            // string *temp = new string(to_string(id) + "\n");
            Task *t = new Task;
            while ((read(socket_fd, buffer, buffer_size) > 0)) {
                // parseRequest(buffer, socket_fd);
                // cout << "read... " << t << endl;
                // cout << *temp << endl;
                t->task += string(buffer);
                // cout << __LINE__ << endl;
            }
            // t->task += string(buffer);
            task_q_.push(*t);
            lockSocket(socket_fd);
            status = getStatus(socket_fd);
            if (status & PendingClose) {
                delSocket(socket_fd);
            } else {
                modSocket(socket_fd, status & ~PendingRead);
            }
            unlockSocket(socket_fd);
        }
    }
}