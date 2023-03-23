#pragma once

#include <SocketBase.hpp>
#define DEFAULT_PORT 8080

class Socket : SocketBase {
  public:
    Socket(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0,
           int port = DEFAULT_PORT, int queue_size = 3);
    ~Socket();
    bool startListen();
    bool stopListen();
    int getClientsCount();

  private:
    int epoll_fd_;
    bool running_ = false;
    void passiveListen();
    void startListen(int timeout);

    void addClientSocket(int socket_fd);
    void delClientSocket(int socket_fd);
};
void sigintHandler(int dummpy);
