#pragma once
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#define INIT_SIZE 4

template <typename T> class MultiThreadQueue {
  public:
    MultiThreadQueue(bool is_deep_copy = true);
    ~MultiThreadQueue();
    T *pull();
    void push(T &x);
    void debug() {
        for (size_t i = top_; i < tail_; ++i) {
            std::cout << data_[i] << ' ' << *data_[i] << std::endl;
        }
    }

  private:
    bool is_deep_copy_ = false;
    size_t resize();
    size_t size_;
    std::mutex m_;
    std::condition_variable cond_;
    T **data_;
    size_t top_;  // represent the front element in the queue
    size_t tail_; // one past the last element in the queue
};

template <typename T> MultiThreadQueue<T>::~MultiThreadQueue() {
    if (is_deep_copy_) {
        for (size_t i = top_; i < tail_; ++i)
            delete data_[i];
    }
    delete[] data_;
}

template <typename T> MultiThreadQueue<T>::MultiThreadQueue(bool is_deep_copy) {
    is_deep_copy_ = is_deep_copy;
    size_ = INIT_SIZE;
    data_ = new T *[INIT_SIZE]; // array of ptr
    top_ = 0;
    tail_ = 0;
}

template <typename T> size_t MultiThreadQueue<T>::resize() {
    T **old_data = data_;
    data_ = new T *[size_ * 2];
    size_t new_i = 0;
    for (size_t old_i = top_; old_i < tail_; ++old_i) {
        data_[new_i] = old_data[old_i]; // only the element ptr is copied
        ++new_i;
    }

    delete[] old_data; // element ptrs should remain intact

    top_ = 0;
    tail_ = new_i;
    size_ *= 2;
    return size_;
}

template <typename T> void MultiThreadQueue<T>::push(T &x) {
    std::unique_lock<std::mutex> lock(m_);

    if (tail_ == size_) {
        // reshape
        // alloc more space
        resize();
    }
    cond_.notify_one();
    if (is_deep_copy_)
        data_[tail_++] = new T(x);
    // using the copy constructor
    else
        data_[tail_++] = &x;
}

template <typename T> T *MultiThreadQueue<T>::pull() {
    std::unique_lock<std::mutex> lock(m_);
    while (top_ == tail_) {
        cond_.wait(lock);
    }
    return data_[top_++];
}