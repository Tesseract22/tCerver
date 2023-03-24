#include "SocketBase.hpp"
#include <Socket.hpp>
#include <TCPServer.hpp>
#include <thread>
#include <unistd.h>

using namespace std;
TCPServer::TCPServer() {
    master_socket_ = new Socket();
    running_ = false;
}
TCPServer::~TCPServer() {
    socket_thread_.join();
    if (running_) {
        for (int i = 0; i < NUM_THREAD; ++i)
            handle_threads_[i].join();
    }
    delete master_socket_;
}

// void TCPServer::startSocket() {
//     master_socket_->lis
// }

void TCPServer::startListen() {

    socket_thread_ = thread(&SocketBase::startListen, master_socket_);
    for (int i = 0; i < NUM_THREAD; ++i) {
        handle_threads_[i] = thread(&TCPServer::waitTask, this);
    }
    running_ = true;
}
void TCPServer::stopListen() { master_socket_->stopListen(); }

void TCPServer::handleRead(int socket_fd) {}

void TCPServer::waitTask() {
    cout << this_thread::get_id() << " start fetching task..." << endl;
    char buffer[1025];
    int buffer_size = 1024;
    while (running_) {
        int socket_fd = master_socket_->getTask();
        master_socket_->lockSocket(socket_fd);
        int status = master_socket_->getClientSocket(socket_fd);
        master_socket_->unlockSocket(socket_fd);
        if (status & PendingRead) {
            while (read(socket_fd, buffer, buffer_size) > 0) {
                cout << buffer << endl;
            }
            master_socket_->lockSocket(socket_fd);
            status = master_socket_->getClientSocket(socket_fd);
            if (status & PendingClose) {
                master_socket_->delClientSocket(socket_fd);
            } else {
                master_socket_->modClientSocket(socket_fd,
                                                status & ~PendingRead);
            }
            master_socket_->unlockSocket(socket_fd);
        }
    }
}