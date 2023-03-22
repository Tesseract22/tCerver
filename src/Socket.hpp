#pragma once

#include <arpa/inet.h>  //close
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h>  //close

#define DEFAULT_PORT 8080

class Socket {
   public:
    Socket(int domain = AF_INET, int type = SOCK_STREAM, int protocol = 0,
           int port = 8080, int queue_size = 3, int client_siz = 10);
    ~Socket();
    bool ready();
    static void cleanUp();
    static bool stop;

   private:
    int socket_fd_;
    sockaddr_in addr_;
    int client_size_;
    int current_size_ = 1;
    pollfd *fds_;
};