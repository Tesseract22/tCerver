#include "Socket.hpp"
#include "MultiThreadQueue.hpp"
#include "SocketBase.hpp"

#include <csignal>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>
using namespace std;

Socket::Socket(int domain, int type, int protocol, int port, int queue_size)
    : SocketBase(domain, type, protocol, port, queue_size), task_q_(true) {
    fcntl(socket_fd_, F_SETFL, O_NONBLOCK); // non blocking
}

Socket::~Socket() {}

bool Socket::startListen() {
    // fds_[i]
    // thread t(&Socket::Listen, this);
    // t.join();
    Socket::startListen(-1);
    return true;
}

bool Socket::stopListen() {
    running_ = false;
    return true;
}

void Socket::startListen(int timeout) {
    char buffer[1025];
    int buffer_size = 1024;

    running_ = true;

    epoll_fd_ = epoll_create(10);
    epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = socket_fd_;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd_, &event) < 0)
        throw runtime_error("failed to add socket to epoll");

    epoll_event revents[10];

    cout << "server started" << '\n';
    while (running_) {
        int num_event = epoll_wait(epoll_fd_, revents, 10, timeout);
        cout << "num_event: " << num_event << endl;
        if (num_event > 0) {

            for (int i = 0; i < num_event; ++i) {
                cout << revents[i].events << " " << revents[i].data.fd << endl;
                int socket_i = revents[i].data.fd;
                if (socket_i == socket_fd_) {
                    int new_socket_fd = accept(socket_fd_, NULL, NULL);
                    if (new_socket_fd < 0)
                        throw runtime_error("failed to estblish new socket");
                    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
                    event.data.fd = new_socket_fd;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, new_socket_fd, &event);
                    addClientSocket(new_socket_fd);
                } else if (revents[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                    lockSocket(socket_i);
                    int status;
                    if ((status = getClientSocket(socket_i) & PendingRead)) {
                        modClientSocket(socket_i, status |= PendingClose);
                    } else {
                        delClientSocket(socket_i);
                    }

                    unlockSocket(socket_i);
                } else if (revents[i].events & EPOLLIN) {
                    // cout << __LINE__ << endl;
                    // if (read(revents[i].data.fd, buffer, buffer_size) > 0) {
                    //     cout << buffer << endl;
                    // }
                    // cout << __LINE__ << endl;
                    lockSocket(socket_i);
                    task_q_.push(socket_i);
                    modClientSocket(socket_i, PendingRead);
                    unlockSocket(socket_i);
                    if (send(socket_i,
                             "HTTP/1.1 200 OK\nContent-Type: "
                             "text/plain\nContent-Length: "
                             "12\n\nHello world!",
                             100, 0) < 0) {
                        throw runtime_error("fail to send");
                    }
                }
            }
        }
    }
}

void Socket::addClientSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        socket_vec_.resize(socket_fd + 1);
        mutex_vec_.resize(socket_fd + 1);
    }
    mutex_vec_[socket_fd] = new mutex;
    socket_vec_[socket_fd] = 1;
    socket_count_++;
}

void Socket::delClientSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw runtime_error("deleting non-existing socket");
    }
    socket_vec_[socket_fd] = 0;
    delete mutex_vec_[socket_fd];
    mutex_vec_[socket_fd] = NULL;
    close(socket_fd);
    socket_count_--;
}

void Socket::modClientSocket(int socket_fd, int act) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw runtime_error("modifying non-existing socket");
    }
    socket_vec_[socket_fd] = act;
}

int Socket::getClientSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw runtime_error("getting non-existing socket");
    }
    return socket_vec_[socket_fd];
}

int Socket::getTask() {
    int *fd = task_q_.pull();
    int res = *fd;
    free(fd);
    cout << "task: " << res << endl;
    return res;
}

int Socket::getClientsCount() { return socket_count_; }

void Socket::lockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_.size())
        throw runtime_error("locking non-existing socket: " +
                            to_string(socket_fd));
    mutex_vec_[socket_fd]->lock();
}

void Socket::unlockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_.size())
        throw runtime_error("unlocking non-existing socket:" +
                            to_string(socket_fd));
    mutex_vec_[socket_fd]->unlock();
}

void Socket::passiveListen() {
    // signal(SIGINT, sigintHandler);

    // while (!stop) {
    //     cout << "waiting for request... (blocked)" << endl;
    //     int result = poll(fds_, current_size_ + 1, -1);
    //     if (stop)
    //         break;
    //     cout << "detect incoming request" << endl;
    //     if (result < 0)
    //         throw runtime_error("socket ready failed");
    //     if (result == 0) {
    //         cout << "what the heck" << endl;
    //         continue;
    //     }
    //     if (fds_[0].revents &
    //         POLLIN) { // there is incoming connection, add that to pds
    //         int client_socket_fd = accept(socket_fd_, NULL, NULL);
    //         if (client_socket_fd > 0)
    //             for (int i = 1; i < client_size_ + 1;
    //                  ++i) { // find the appropriate place for the new socket
    //                 if (fds_[i].fd == 0) { // found an unsed socket
    //                     fds_[i].fd = client_socket_fd;
    //                     fds_[i].events = POLLIN | POLLRDHUP;
    //                     current_size_++;
    //                     cout << "accepted on socket index: " << i << endl;
    //                     break;
    //                 }
    //             }
    //     }
    //     for (int i = 0; i < client_size_; ++i)
    //         cout << "fds[" << i << "]: " << fds_[i].fd << ' ' <<
    //         fds_[i].events
    //              << ' ' << fds_[i].revents << endl;
    //     for (int i = 1; i < client_size_; ++i) {
    //         if (fds_[i].fd > 0 && fds_[i].revents & (POLLIN | POLLRDHUP)) {
    //             cout << "connection from " << i << endl;
    //             char buffer[1024];
    //             int buffer_size = read(fds_[i].fd, buffer, 1024);
    //             cout << "buffer_size: " << buffer_size << endl;
    //             if (buffer_size <= 0) {
    //                 close(fds_[i].fd);
    //                 fds_[i].fd = 0;
    //                 fds_[i].events = 0;
    //                 fds_[i].revents = 0;
    //                 current_size_--;
    //             } else {
    //                 char msg[100] =
    //                     "HTTP/1.1 200 OK\nContent-Type: "
    //                     "text/plain\nContent-Length: 12\n\nHello world!";
    //                 if (send(fds_[i].fd, msg, 100, 0) < 0) {
    //                     throw runtime_error("failed to send message");
    //                 }
    //                 cout << "echoing message..." << endl;
    //                 cout << "content: \n" << buffer << endl;
    //             }
    //         }
    //     }
    //     cout << "\n\n" << endl;
    // }
}