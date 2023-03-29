#pragma once
#include <MultiThreadQueue.hpp>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>
struct Task {
    std::string task;
    int socket_fd;
};

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

enum Action {
    Open = 0b1000,
    PendingRead = 0b001,
    PendingClose = 0b010,
    PendingWrite = 0b100,
};

class EPoll {
  public:
    EPoll(std::mutex *m_, int master_socket_fd, std::vector<int> *sockt_vec,
          std::vector<std::mutex *> *mutex_vec,
          MultiThreadQueue<Task *> *task_q_);
    EPoll(EPoll &&other);
    EPoll(const EPoll &X) = default;

    void wait();

  private:
    void addSocket(int socket_fd_);
    void delSocket(int socket_fd_);
    void modSocket(int socket_fd_, int act);
    int getSocket(int socket_fd_);

    void lockSocket(int socket_fd);
    void unlockSocket(int socket_fd);

    std::mutex *m_;
    int master_socket_fd_;
    std::vector<std::mutex *> *mutex_vec_;
    int epoll_fd_;
    epoll_event temp_event_;
    // The socket file descriptor is directly used as indexes
    // The value in the vector represent the status of the socket
    std::vector<int> *socket_vec_;

    MultiThreadQueue<Task *> *task_q_;
};