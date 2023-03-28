#include "MultiThreadQueue.hpp"
#include <EPoll.hpp>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

using namespace std;
EPoll::EPoll(mutex *m, int master_socket_fd, vector<int> *socket_vec,
             vector<mutex *> *mutex_vec, MultiThreadQueue<Task *> *task_q)
    : m_(m),
      master_socket_fd_(master_socket_fd),
      mutex_vec_(mutex_vec),
      epoll_fd_(epoll_create(1)),
      socket_vec_(socket_vec),
      task_q_(task_q) {
    temp_event_.events = EPOLLIN | EPOLLRDHUP;
    temp_event_.data.fd = master_socket_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, master_socket_fd_, &temp_event_) <
        0)
        throw runtime_error("failed to add socket to epoll");
}

EPoll::EPoll(EPoll &&other) {
    other.m_ = m_;
    other.master_socket_fd_ = other.master_socket_fd_;
    other.mutex_vec_ = mutex_vec_;
    other.epoll_fd_ = epoll_fd_;
    other.temp_event_.events = temp_event_.events;
    other.temp_event_.data = temp_event_.data;
    other.socket_vec_ = socket_vec_;
    other.task_q_ = task_q_;
}

void EPoll::wait() {
    char buffer[1025];
    int buffer_size = 1024;

    epoll_event revents[10];

    cout << "listening thread started" << '\n';
    while (true) {
        int num_event = epoll_wait(epoll_fd_, revents, 10, -1);
        cout << "num_event: " << num_event << endl;
        if (num_event > 0) {

            for (int i = 0; i < num_event; ++i) {
                cout << revents[i].events << " " << revents[i].data.fd << endl;
                int socket_i = revents[i].data.fd;
                if (socket_i == master_socket_fd_) {
                    int new_socket_fd = accept(master_socket_fd_, NULL, NULL);
                    if (new_socket_fd < 0)
                        throw runtime_error("failed to estblish new socket");
                    temp_event_.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
                    temp_event_.data.fd = new_socket_fd;
                    m_->lock();
                    addSocket(new_socket_fd);
                    m_->unlock();
                } else if (revents[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                    lockSocket(socket_i);
                    int status;
                    if ((status = getSocket(socket_i) & PendingRead)) {
                        modSocket(socket_i, status |= PendingClose);
                    } else {
                        delSocket(socket_i);
                    }
                    unlockSocket(socket_i);

                } else if (revents[i].events & EPOLLIN) {

                    {
                        Task *t = new Task;

                        while (read(socket_i, buffer, buffer_size) >= 0) {
                            t->task += string(buffer);
                        }
                        buffer[buffer_size] = '\0';
                        lockSocket(socket_i);
                        cout
                            << "listening thread: task added, size of request: "
                            << t->task.size() << endl;
                        task_q_->push(t);
                        modSocket(socket_i, PendingRead);
                        unlockSocket(socket_i);

                        // if (send(socket_i,
                        //          "HTTP/1.1 200 OK\nContent-Type: "
                        //          "text/plain\nContent-Length: "
                        //          "12\n\n",
                        //          100, 0) < 0) {
                        //     throw runtime_error("fail to send");
                        // }
                    }
                }
            }
        }
    }
}
void EPoll::addSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_->size()) {
        socket_vec_->resize(socket_fd + 1);
        mutex_vec_->resize(socket_fd + 1);
    }
    if (socket_vec_->operator[](socket_fd) != 0)
        throw runtime_error("socket already exist");
    if (mutex_vec_->operator[](socket_fd) == NULL)
        mutex_vec_->operator[](socket_fd) = new mutex;

    temp_event_.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    temp_event_.data.fd = socket_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd, &temp_event_);
    // @important!
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);
    socket_vec_->operator[](socket_fd) = Open;
}

void EPoll::delSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_->size()) {
        throw runtime_error("deleting non-existing socket");
    }
    socket_vec_->operator[](socket_fd) = 0;
    close(socket_fd);
}

void EPoll::modSocket(int socket_fd, int act) {
    if ((size_t)socket_fd >= socket_vec_->size()) {
        throw runtime_error("modifying non-existing socket");
    }
    socket_vec_->operator[](socket_fd) = act;
}

int EPoll::getSocket(int socket_fd) {
    if ((size_t)socket_fd >= socket_vec_->size()) {
        throw runtime_error("getting non-existing socket");
    }
    return socket_vec_->operator[](socket_fd);
}

void EPoll::lockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_->size())
        throw runtime_error("locking non-existing socket: " +
                            to_string(socket_fd));
    mutex_vec_->operator[](socket_fd)->lock();
}

void EPoll::unlockSocket(int socket_fd) {
    if ((size_t)socket_fd >= mutex_vec_->size())
        throw runtime_error("unlocking non-existing socket:" +
                            to_string(socket_fd));
    mutex_vec_->operator[](socket_fd)->unlock();
}