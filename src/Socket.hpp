#pragma once

#include <MultiThreadQueue.hpp>
#include <SocketBase.hpp>
#define DEFAULT_PORT 8080

class Socket : public SocketBase {
  public:
    Socket(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0,
           int port = DEFAULT_PORT, int queue_size = 3);
    ~Socket();
    bool startListen();
    bool stopListen();
    int getClientsCount();
    int getTask();
    int getClientSocket(int socket_fd);
    void addClientSocket(int socket_fd);
    void delClientSocket(int socket_fd);
    void modClientSocket(int socket_fd, int act);

    void lockSocket(int socket_fd);
    void unlockSocket(int socket_fd);

  private:
    int epoll_fd_;
    bool running_ = false;
    MultiThreadQueue<int> task_q_;

    void passiveListen();
    void startListen(int timeout);
};
void sigintHandler(int dummpy);
