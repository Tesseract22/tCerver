#pragma once
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <thread>
#include <unistd.h> //close
#include <vector>
#define DEFAULT_PORT 8080

enum Action {
    Open = 0b1000,
    PendingRead = 0b001,
    PendingClose = 0b010,
    PendingWrite = 0b100,
};
class SocketBase {
  public:
    SocketBase(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0,
               int port = DEFAULT_PORT, int queue_size = 3);
    virtual ~SocketBase();
    bool virtual startListen() = 0;
    bool virtual stopListen() = 0;
    int virtual getTask() = 0;
    int virtual getClientSocket(int socket_fd) = 0;
    void virtual addClientSocket(int socket_fd) = 0;
    void virtual delClientSocket(int socket_fd) = 0;
    void virtual modClientSocket(int socket_fd, int act) = 0;
    void virtual lockSocket(int socket_fd) = 0;
    void virtual unlockSocket(int socket_fd) = 0;
    inline int getClientsCount();

  protected:
    int socket_fd_;
    sockaddr_in addr_;
    std::thread listen_thread;
    std::vector<int> socket_vec_;
    std::vector<std::mutex *> mutex_vec_;
    int socket_count_ = 1;
};