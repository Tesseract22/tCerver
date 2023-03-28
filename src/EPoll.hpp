#pragma once
#include <MultiThreadQueue.hpp>
#include <mutex>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>
struct Task {
    std::string task;
    int socket_fd;
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
          MultiThreadQueue<Task> *task_q_);
    EPoll(EPoll &&other);
    EPoll(const EPoll &X) = default;
    void addSocket(int socket_fd_);
    void delSocket(int socket_fd_);
    void modSocket(int socket_fd_, int act);
    int getSocket(int socket_fd_);
    void wait();

    void lockSocket(int socket_fd);
    void unlockSocket(int socket_fd);

  private:
    std::mutex *m_;
    int master_socket_fd_;
    std::vector<std::mutex *> *mutex_vec_;
    int epoll_fd_;
    epoll_event temp_event_;
    // The socket file descriptor is directly used as indexes
    // The value in the vector represent the status of the socket
    std::vector<int> *socket_vec_;

    MultiThreadQueue<Task> *task_q_;
};