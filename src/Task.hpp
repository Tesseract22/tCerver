#pragma once
#include "Scheduler.hpp"
#include <condition_variable>
#include <coroutine>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
template <typename T> class Task {
  public:
    // ~Task<T>() {}
    // Task<T>(std::cor) {}
    Task<T>(Task<T> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})) {}
    Task<T>(Task<T> &) = delete;
    Task<T> &operator=(Task<T> &) = delete;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::mutex m;
        // std::optional<T> result;
        std::shared_ptr<T> result = std::make_shared<T>();
        std::shared_ptr<bool> finished = std::make_shared<bool>(false);
        std::condition_variable cond;
        std::vector<std::coroutine_handle<>> others;
        Scheduler initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule");
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            std::unique_lock<std::mutex> lock(m);
            *finished = true;
            for (auto &o : others) {
                Scheduler::enqueue(o);
            }
            // DEBUG_PRINT("enqueue");
            cond.notify_all();
            return {};
        }
        void unhandled_exception() { DEBUG_PRINT("exception"); }
        Task get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this),
                    result, finished};
        }
        // void return_void() {}
        void return_value(T value) { *result = std::move(value); }
        Scheduler yield_value(T value) {
            // if (result.has_value())
            //     throw std::runtime_error("yield value not consumed");
            for (auto &o : others)
                Scheduler::enqueue(o);
            others.clear();
            *result = std::move(value);
            return {};
        }
    };

    Task<T>(std::coroutine_handle<promise_type> h, std::shared_ptr<T> result,
            std::shared_ptr<bool> finished) {
        handle_ = h;
        result_ = result;
        finished_ = finished;
    }

    bool constexpr await_ready() const { return false; }
    bool await_suspend(std::coroutine_handle<> handle) {
        std::unique_lock<std::mutex> lock(handle_.promise().m);

        // DEBUG_PRINT("awaited ");
        if (*finished_) {
            return false; // resumes current coroutine
        } else {
            handle_.promise().others.push_back(handle);
            return true;
        }
    }
    T await_resume() const { return std::move(*result_); }

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<T> result_;
    std::shared_ptr<bool> finished_;
};

template <> class Task<void> {
  public:
    // ~Task<T>() {
    //     if (handle_)
    //         handle_.destroy();
    // }

    Task<void>(Task<void> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})) {}
    Task<void>(Task<void> &) = delete;
    Task<void> &operator=(Task<void> &) = delete;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::mutex m;
        volatile bool finished = false;
        std::condition_variable cond;
        Scheduler initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule");
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            std::unique_lock<std::mutex> lock(m);
            finished = true;
            cond.notify_all();
            return {};
        }
        void unhandled_exception() {}
        Task get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        void return_void() {}
    };

    Task<void>(std::coroutine_handle<promise_type> h) { handle_ = h; }

    bool constexpr await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        // DEBUG_PRINT("await suspend");
        std::unique_lock<std::mutex> lock(handle_.promise().m);

        handle_.promise().cond.wait(
            lock, [this] { return handle_.promise().finished; });
        // DEBUG_PRINT("never reached?");
        handle.resume();
    }
    void await_resume() const {}

    std::coroutine_handle<promise_type> handle_;
};
