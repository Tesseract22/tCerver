#pragma once
#include "SocketBase.hpp"
#include <MultiThreadQueue.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
// TCP server implementation

#define NUM_THREAD 2

class TCPServer {
  public:
    TCPServer();
    ~TCPServer();
    void startListen();
    void stopListen();

    void handleRead(int socket_fd);
    void handleWrite(int socket_fd);

  private:
    bool running_;
    void waitTask();
    std::thread handle_threads_[NUM_THREAD];

    std::thread socket_thread_;
    SocketBase *master_socket_;
};
