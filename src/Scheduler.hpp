#pragma once
#include "MultiThreadQueue.hpp"
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#define DEBUG_PRINT(info)                                                      \
    std::cout << "[" << std::this_thread::get_id() << "] " << info << std::endl;
class Scheduler {
  public:
    // friend class Task;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        if (running_)
            queue_.push(handle);
        else
            throw std::runtime_error("scheduler not started");
    }
    void await_resume() const noexcept {}
    size_t static addThreads(size_t add_threads_ct) {

        std::lock_guard<std::mutex> lock(m_);

        for (size_t i = 0; i < add_threads_ct; ++i) {
            std::thread t(&Scheduler::thread_start);
            t.detach();
            threads_.push_back(std::move(t));
        }

        return threads_.size();
    }
    void static start(size_t i = 5) {
        thread_ct_ = i;
        if (!running_) {
            running_ = true;
            addThreads(thread_ct_);
            std::thread t([] {
                std::unique_lock<std::mutex> thread_lock(m_);
                std::condition_variable cond;
                cond.wait(thread_lock, [] { return !running_; });
            });
            t_ = std::move(t);
        } else {
            throw std::runtime_error("scheduler already running");
        }
    }
    void static join() { t_.join(); }
    void static stop() {
        if (running_) {
            std::unique_lock<std::mutex> thread_lock(m_);
            std::condition_variable cond;
            running_ = true;
            cond.notify_all();
        } else {
            throw std::runtime_error("scheduler not startedd");
        }
    }
    void static enqueue(const std::coroutine_handle<> &handle) {
        queue_.push(handle);
    }

  private:
    // Scheduler() = default;
    void static thread_start() {
        // DEBUG_PRINT("thread starting");
        while (1) {
            auto handle = queue_.pull();
            handle.resume();
            // DEBUG_PRINT(handle.done());
        }
    }
    static std::thread t_;
    static std::mutex m_;
    static size_t thread_ct_;
    static std::vector<std::thread> threads_;
    static MultiThreadQueue<std::coroutine_handle<>> queue_;
    static volatile bool running_;
};
