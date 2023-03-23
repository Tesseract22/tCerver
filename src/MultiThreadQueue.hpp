#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

#define INIT_SIZE 4

template <typename T> class MultiThreadQueue {
  public:
    MultiThreadQueue();
    ~MultiThreadQueue();
    T &pull();
    void push(const T &x);

  private:
    size_t resize();
    size_t size_;
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
    data_ = new T[INIT_SIZE];
    top_ = 0;
    tail_ = 0;
}

template <typename T> size_t MultiThreadQueue<T>::resize() {
    T *old_data = data_;
    data_ = new T[size_ * 2];
    size_t new_i = 0;
    for (size_t old_i = top_; old_i < size_; ++old_i, ++new_i) {
        data_[new_i] = old_data[old_i];
    }
    delete[] old_data;

    top_ = 0;
    tail_ = new_i;
    size_ *= 2;
    return size_;
}

template <typename T> void MultiThreadQueue<T>::push(const T &x) {
    if (tail_ == size_) {
        // reshape
        // alloc more space
        resize();
    }
    data_[tail_++] = T(x); // using the copy constructor
}

template <typename T> T &MultiThreadQueue<T>::pull() {
    if (top_ != tail_)
        return data_[top_++];
    throw "";
}