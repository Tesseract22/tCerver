#pragma once
#include "SocketBase.hpp"
#include <MultiThreadQueue.hpp>
#include <string>
#include <vector>
// TCP server implementation

class TCPServer {
  public:
    TCPServer();
    void startListen();
    void stopListen();

  private:
    MultiThreadQueue<std::string> task_queue_;
    SocketBase *master_socket_;
};
