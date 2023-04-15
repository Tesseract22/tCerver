#pragma once
#include "MultiThreadQueue.hpp"
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#define DEBUG_PRINT(info)                                                      \
    std::cerr << "[" << std::this_thread::get_id() << "] " << info << std::endl;

class Scheduler {
  public:
    // friend class Task;

    Scheduler(size_t count)

        : queue_(
              std::make_shared<MultiThreadQueue<std::coroutine_handle<>>>()) {
        for (size_t i = 0; i < count; ++i) {
            threads_.emplace_back(&Scheduler::thread_start, this);
        }
        list_.push_back(this);
    }

    ~Scheduler() {
        for (auto &t : threads_) {
            t.join();
        }
    }

    Scheduler(const Scheduler &other) {
        threads_ = std::vector<std::thread>(0);
        queue_ = other.queue_;
        running_ = other.running_;
    }
    void enqueue(const std::coroutine_handle<> &handle) {
        queue_->push(handle);
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) { queue_->push(handle); }
    void await_resume() const noexcept {}

    void static SIGINT_HANDLER(int dummy) {
        for (auto s : list_) {
            s->running_ = false;
            for (auto &t : s->threads_) {
                s->enqueue(std::coroutine_handle<>());
            }
        }
    }

  private:
    // Scheduler() = default;
    static std::vector<Scheduler *> list_;
    void thread_start() {
        while (running_) {
            auto handle = queue_->pull();
            if (!running_)
                break;
            handle.resume();
            // DEBUG_PRINT(handle.done());
        }
    }
    volatile bool running_ = true;
    std::vector<std::thread> threads_;
    std::shared_ptr<MultiThreadQueue<std::coroutine_handle<>>> queue_;
    // static std::vector<Scheduler*>
};
