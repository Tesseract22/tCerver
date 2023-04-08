#include "MultiThreadQueue.hpp"
#include "Scheduler.hpp"
#include "TCPServer.hpp"
#include "Task.hpp"
#include "utilities.hpp"
#include <cerrno>
#include <coroutine>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
using namespace std;
// reworking Epoll using coroutine
// basically, change the Epoll wait function to a coroutine.
// the time consuming read and write is preformed on an awaiter
// we can bind a callback function to the awaiter,
// we can use this callback to parse the response

void TCPServer::EPoll::stop() { running_ = false; }

TCPServer::EPoll::EPoll(mutex &m, int master_socket_fd, vector<int> &socket_vec,
                        vector<mutex *> &mutex_vec,
                        const function<Task<Response>(string &)> &callback)
    : m_(m),
      master_socket_fd_(master_socket_fd),
      mutex_vec_(mutex_vec),
      socket_vec_(socket_vec),
      handleReqeust_(callback) {
    epoll_fd_ = epoll_create(1);
    memset(&temp_event_, 0, sizeof(epoll_event));
    temp_event_.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    temp_event_.data.fd = master_socket_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, master_socket_fd_, &temp_event_) <
        0)
        throw runtime_error("failed to add socket to epoll");
}

Task<void> TCPServer::EPoll::wait(int dummpy_fd) {

    // char buffer[1025];
    // int buffer_size = 1024;
    // memset(buffer, 0, 1025);

    epoll_event revents[20];

#if DEBUG
    DEBUG_PRINT("start listening")
#endif
    if (!running_) {
        temp_event_.events = EPOLLIN;
        temp_event_.data.fd = dummpy_fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, dummpy_fd, &temp_event_) < 0) {
            throw runtime_error("failed to add dummy fd to epoll");
        }
    }
    running_ = true;
    while (true) {
        int num_event = epoll_wait(epoll_fd_, revents, 20, -1);
#if DEBUG
        DEBUG_PRINT("num_event: " << num_event)
#endif
        if (!running_) {
            co_return;
        }
        if (num_event < 0) {
            throw SocketException("epoll_wait failed, retrying");
        }
        int bytes;
        if (num_event > 0) {
            for (int i = 0; i < num_event; ++i) {
                int socket_i = revents[i].data.fd;
                if (socket_i == master_socket_fd_) {
                    int new_socket_fd = accept(master_socket_fd_, NULL, NULL);
                    if (new_socket_fd < 0) {
                        if (errno !=
                            11) // https://stackoverflow.com/questions/39145357/python-error-socket-error-errno-11-resource-temporarily-unavailable-when-s
                            throw SocketException(
                                "failed to establish new socket");
                    } else {
                        m_.lock();
                        addSocket(new_socket_fd);
                        m_.unlock();
                    }
#if DEBUG
                    DEBUG_PRINT("new socket established: " << new_socket_fd)
#endif

                } else if (revents[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                    lockSocket(socket_i);
                    int status;
                    if ((status = getSocket(socket_i) & PendingRead)) {
                        modSocket(socket_i, status |= PendingClose);
                    } else {
                        delSocket(socket_i);
                    }
#if DEBUG
                    DEBUG_PRINT("socket closed: " << socket_i)
#endif
                    unlockSocket(socket_i);

                } else if (revents[i].events & EPOLLIN) {
#if DEBUG
                    DEBUG_PRINT("incoming read from: " << socket_i)
#endif
                    [socket_i, this]() -> Task<void> {
                        char buffer[1024];
                        string result =
                            utility::readSocket(socket_i, buffer, 1023);
                        Response response = co_await handleReqeust_(result);
                        utility::writeSocket(socket_i, response.str.data(),
                                             response.str.length());
                        if (response.fd > 0) {
                            utility::writeFileSocket(socket_i, response.fd);
                            close(response.fd);
                        }
                    }();
                }
            }
        }
    }
}
void TCPServer::EPoll::addSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        socket_vec_.resize(socket_fd + 1);
    }
    if (socket_vec_[socket_fd] != 0)
        throw SocketException("socket already exist");

    temp_event_.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    temp_event_.data.fd = socket_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd, &temp_event_);
    // @important!
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);
    socket_vec_[socket_fd] = Open;
}

void TCPServer::EPoll::delSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw SocketException("deleting non-existing socket");
    }
    socket_vec_[socket_fd] = Closed;
    close(socket_fd);
}

void TCPServer::EPoll::modSocket(int socket_fd, int act) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw SocketException("modifying non-existing socket");
    }
    socket_vec_[socket_fd] = act;
}

int TCPServer::EPoll::getSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_.size()) {
        throw SocketException("getting non-existing socket");
    }
    return socket_vec_[socket_fd];
}
void TCPServer::EPoll::lockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_.size())
        throw SocketException("locking non-existing socket: " +
                              to_string(socket_fd));
    mutex_vec_[socket_fd]->lock();
}

void TCPServer::EPoll::unlockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_.size())
        throw SocketException("unlocking non-existing socket:" +
                              to_string(socket_fd));
    mutex_vec_[socket_fd]->unlock();
}