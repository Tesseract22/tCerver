#pragma once
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <thread>
#include <unistd.h> //close
#include <vector>
#define DEFAULT_PORT 8080
class SocketBase {
  public:
    SocketBase(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0,
               int port = DEFAULT_PORT, int queue_size = 3);
    ~SocketBase();
    bool virtual startListen() = 0;
    bool virtual stopListen() = 0;
    int getClientsCount();

  protected:
    int socket_fd_;
    sockaddr_in addr_;
    std::thread listen_thread;
    std::vector<int> socket_vec_;
    int socket_count_ = 1;
};