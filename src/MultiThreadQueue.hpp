#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>

#include <vector>
#define INIT_SIZE 1

template <typename T> class MultiThreadQueue {
  public:
    MultiThreadQueue();
    ~MultiThreadQueue();
    T &pull();
    void push(const T &x);
    // void debug() {

    // }

  private:
    size_t resize();
    size_t size_;
    size_t curr_size_;
    std::mutex m_;
    std::condition_variable cond_;
    T *data_;
    size_t top_;  // represent the front element in the queue
    size_t tail_; // one past the last element in the queue
};

template <typename T> MultiThreadQueue<T>::~MultiThreadQueue() {
    delete[] data_;
}

template <typename T> MultiThreadQueue<T>::MultiThreadQueue() {
    size_ = INIT_SIZE;
    data_ = new T[INIT_SIZE]; // array of ptr
    curr_size_ = 0;
    top_ = 0;
    tail_ = 0;
}

template <typename T> size_t MultiThreadQueue<T>::resize() {
    T *old_data = data_;
    data_ = new T[size_ * 2];
    size_t new_i = 0;
    size_t old_i = top_;
    while (new_i < curr_size_) {
        data_[new_i++] = old_data[old_i++];
        old_i %= size_;
    }

    delete[] old_data; // element ptrs should remain intact

    top_ = 0;
    tail_ = new_i;
    size_ *= 2;
    return size_;
}

template <typename T> void MultiThreadQueue<T>::push(const T &x) {
    std::unique_lock<std::mutex> lock(m_);
    if (size_ == curr_size_) {
        // reshape
        // alloc more space
        resize();
    }
    data_[tail_++] = T(x);
    tail_ %= size_;
    curr_size_++;
    lock.unlock();
    cond_.notify_one();
}

template <typename T> T &MultiThreadQueue<T>::pull() {
    std::unique_lock<std::mutex> lock(m_);
    cond_.wait(lock, [this] { return curr_size_ != 0; });
    T &ret = data_[top_++];
    top_ %= size_;
    curr_size_--;
    return ret;
}