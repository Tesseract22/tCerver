#include "Socket.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;
bool Socket::stop = true;
void Socket::cleanUp() { stop = false; }

Socket::~Socket() {
    cout << "closing" << endl;
    close(socket_fd_);
    free(fds_);
}

Socket::Socket(int domain, int type, int protocol, int port, int queue_size,
               int client_size) {
    socket_fd_ = socket(domain, type, protocol);
    addr_.sin_family = domain;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port);
    int on = 1;
    setsockopt(socket_fd_, SOL_SOCKET,  SO_REUSEADDR,
                  &on, sizeof(on));

    client_size_ = client_size;
    fds_ = new pollfd[client_size + 1];
    memset(fds_, 0, sizeof(pollfd) * (client_size + 1));
    fds_[0].fd = socket_fd_;
    fds_[0].events = POLLIN;

    if (bind(socket_fd_, (sockaddr *)&addr_, sizeof(addr_)) < 0) {
        cout << errno << endl;
        throw runtime_error("bind failed");
    }

    if (listen(socket_fd_, queue_size) < 0) throw runtime_error("listen failed");
}

bool Socket::ready() {
    // fds_[i]
    while (stop) {
        cout << "waiting for request... (blocked)" << endl;
        int result = poll(fds_, current_size_ + 1, -1);
        if (!stop) break;
        cout << "detect incoming request" << endl;
        if (result < 0) throw runtime_error("socket ready failed");
        // if (result == 0) {
        //     continue;
        // }
        if (fds_[0].revents &
            POLLIN) {  // there is incoming connection, add that to pds
            int client_socket_fd = accept(socket_fd_, NULL, NULL);

            for (int i = 1; i < client_size_ + 1;
                 ++i) {                 // find the appropriate place for the new socket
                if (fds_[i].fd == 0) {  // found an unsed socket
                    fds_[i].fd = client_socket_fd;
                    fds_[i].events = POLLIN;
                    current_size_++;
                    cout << "accepted on socket index: " << i << endl;
                    break;
                }
            }
        }
        cout << fds_[0].revents << endl;
        for (int i = 1; i < client_size_; ++i) {
            cout << fds_[i].revents << endl;
            if (fds_[i].fd > 0 && fds_[i].revents & POLLIN) {
                cout << "connection from " << i << endl;
                char buffer[1024];
                int buffer_size = read(fds_[i].fd, buffer, 1024);
                cout << "buffer_size: " << buffer_size << endl;
                if (buffer_size <= 0) {
                    close(fds_[i].fd);
                    fds_[i].fd = 0;
                    fds_[i].events = 0;
                    fds_[i].revents = 0;
                    current_size_--;
                } else {
                    char msg[100] = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
                    if (send(fds_[i].fd, msg, 100, 0) < 0) {
                        throw runtime_error("failed to send message");
                    }
                    cout << "echoing message..." << endl;
                    cout << "content: \n"
                         << buffer << endl;
                }
            }
        }
        cout << "\n\n"
             << endl;
    }

    return true;
}