#pragma once
#include "Exceptions.hpp"
#include "Scheduler.hpp"
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>

struct SameThreadResumer {
    std::vector<std::coroutine_handle<>> resumers;
    bool resuming;
    explicit SameThreadResumer(std::vector<std::coroutine_handle<>> &&r)
        : resumers(r), resuming(true) {}
    explicit SameThreadResumer() : resumers(), resuming(false) {}

    bool await_ready() const {
        for (auto &h : resumers)
            h.resume();
        return resuming;
    }
    void await_suspend(std::coroutine_handle<> handle) const noexcept {}
    void await_resume() const {}
};

template <typename T> class Task {
  public:
    // ~Task<T>() {}
    // Task<T>(std::cor) {}
    Task<T>(Task<T> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          result_(std::exchange(other.result_, {})),
          finished_(std::exchange(other.finished_, {})),
          m_(std::exchange(other.m_, {})) {}

    Task<T>(Task<T> &) = default;
    Task<T> &operator=(Task<T> &) = default;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
        std::shared_ptr<T> result = std::make_shared<T>();
        std::shared_ptr<bool> finished = std::make_shared<bool>(false);
        std::vector<std::coroutine_handle<>> others;
        Scheduler *scheduler = nullptr;
        std::suspend_never initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule")
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            m->lock();
            *finished = true;
            if (scheduler) {
                for (auto &o : others)
                    scheduler->enqueue(o);
            } else {
                for (auto &o : others)
                    o.resume();
            }
            // DEBUG_PRINT("enqueue");
            m->unlock();
            return {};
        }
        void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const SocketException &e) {
                DEBUG_PRINT(e.what());
            }
        }

        Task get_return_object() {
            // std::cout << test << std::endl;
            return {std::coroutine_handle<promise_type>::from_promise(*this),
                    result, finished, m};
        }
        // void return_void() {}
        void return_value(T value) { *result = std::move(value); }
        std::suspend_always yield_value(T value) {
            m->lock();
            // resume "other" coroutine that are waiting us
            if (scheduler) {
                for (auto &o : others)
                    scheduler->enqueue(o);
            } else {
                for (auto &o : others)
                    o.resume();
            }
            others.clear();
            *result = std::move(value);
            m->unlock();
            return {};
        }
        template <typename T_>
        constexpr Task<T_> &await_transform(Task<T_> &task) const {
            return task;
        }
        Scheduler &await_transform(Scheduler &s) {
            scheduler = &s;
            return s;
        }
    };

    Task<T>(const std::coroutine_handle<promise_type> &h,
            const std::shared_ptr<T> &result,
            const std::shared_ptr<bool> &finished,
            const std::shared_ptr<std::mutex> &m) {
        handle_ = h;
        result_ = result;
        finished_ = finished;
        m_ = m;
        // std::cerr << m << std::endl;
    }

    bool constexpr await_ready() const { return false; }
    bool await_suspend(std::coroutine_handle<> handle) const noexcept {
        // std::cerr << m_ << std::endl;
        m_->lock();

        // DEBUG_PRINT("awaited ");
        if (*finished_) {
            m_->unlock();
            return false; // resumes current coroutine
        } else {
            handle_.promise().others.push_back(handle);
            m_->unlock();
            return true;
        }
    }
    T await_resume() const { return std::move(*result_); }

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<T> result_ = nullptr;
    std::shared_ptr<bool> finished_ = nullptr;
    std::shared_ptr<std::mutex> m_ = nullptr;
};

template <> class Task<void> {
  public:
    // ~Task<T>() {}
    // Task<T>(std::cor) {}
    Task<void>() = default;
    Task<void>(Task<void> &&other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          finished_(std::exchange(other.finished_, {})),
          m_(std::exchange(other.m_, {})) {}
    Task<void>(Task<void> &) = default;
    Task<void> &operator=(Task<void> &) = default;
    // https://www.bennyhuo.com/2022/03/11/cpp-coroutines-02-generator/

    struct promise_type {
        std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
        std::shared_ptr<bool> finished = std::make_shared<bool>(false);
        std::vector<std::coroutine_handle<>> others;
        Scheduler *scheduler = nullptr;
        std::suspend_never initial_suspend() noexcept {
            // DEBUG_PRINT("await schedule")
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            m->lock();
            *finished = true;
            for (auto &o : others) {
                scheduler->enqueue(o);
            }
            // DEBUG_PRINT("enqueue");
            m->unlock();
            return {};
        }
        void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const SocketException &e) {
                std::cerr << e.what() << std::endl;
            }
        }
        Task<void> get_return_object() {
            // std::cout << test << std::endl;
            return {std::coroutine_handle<promise_type>::from_promise(*this),
                    finished, m};
        }
        // void return_void() {}
        void return_void() {}
        std::suspend_always yield_value() {
            // if (result.has_value())
            //     throw std::runtime_error("yield value not consumed");
            m->lock();
            for (auto &o : others)
                scheduler->enqueue(o);
            others.clear();
            m->unlock();
            return {};
        }

        Scheduler &await_transform(Scheduler &s) {
            scheduler = &s;
            return s;
        }

        template <typename T_>
        constexpr Task<T_> await_transform(Task<T_> task) const {
            return task;
        }
    };
    Task<void>(const std::coroutine_handle<promise_type> &h,
               const std::shared_ptr<bool> &finished,
               const std::shared_ptr<std::mutex> &m) {
        handle_ = h;
        finished_ = finished;
        m_ = m;
    }

    bool constexpr await_ready() const { return false; }
    bool await_suspend(std::coroutine_handle<> handle) const {

        // DEBUG_PRINT("awaited ");
        m_->lock();
        if (*finished_) {
            m_->unlock();
            return false; // resumes current coroutine
        } else {
            handle_.promise().others.push_back(handle);
            m_->unlock();
            return true;
        }
    }
    void await_resume() const {}

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<bool> finished_;
    std::shared_ptr<std::mutex> m_;
};
