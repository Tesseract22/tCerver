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
    // void debug() {

    // }

  private:
    bool is_deep_copy_ = false;
    size_t resize();
    size_t size_;
    size_t curr_size_;
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
    curr_size_ = 0;
    top_ = 0;
    tail_ = 0;
}

template <typename T> size_t MultiThreadQueue<T>::resize() {
    T **old_data = data_;
    data_ = new T *[size_ * 2];
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

template <typename T> void MultiThreadQueue<T>::push(T &x) {
    std::unique_lock<std::mutex> lock(m_);
    if (size_ == curr_size_) {
        // reshape
        // alloc more space
        resize();
    }
    if (is_deep_copy_)
        data_[tail_++] = new T(x);
    // using the copy constructor
    else
        data_[tail_++] = &x;

    tail_ %= size_;
    curr_size_++;
    cond_.notify_all();
}

template <typename T> T *MultiThreadQueue<T>::pull() {
    std::unique_lock<std::mutex> lock(m_);

    cond_.wait(lock, [this] { return curr_size_ != 0; });

    T *ret = data_[top_++];
    top_ %= size_;
    curr_size_--;
    return ret;
}