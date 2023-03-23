#include <SocketBase.hpp>
#include <iostream>
#include <stdexcept>

using namespace std;
SocketBase::SocketBase(int domain, int type, int protocol, int port,
                       int queue_size) {
    socket_fd_ = socket(domain, type, protocol);
    addr_.sin_family = domain;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port);
    int on = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(socket_fd_, (sockaddr *)&addr_, sizeof(addr_)) < 0) {
        throw runtime_error("bind failed");
    }

    if (listen(socket_fd_, queue_size) < 0)
        throw runtime_error("listen failed");
}

SocketBase::~SocketBase() { close(socket_fd_); }

int SocketBase::getClientsCount() { return socket_count_; }