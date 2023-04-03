#include <coroutine>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct ReturnObject {
    struct promise_type {
        int value;
        ReturnObject get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        std::suspend_always await_transform(int value) {
            this->value = value;
            return {};
        }
        void unhandled_exception() {}
        int return_value(int a) { return 0; }
    };
    int next() {
        handle.resume();
        return handle.promise().value;
    }
    std::coroutine_handle<promise_type> handle;
};

struct Awaiter {
    std::coroutine_handle<> *hp_;
    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) { *hp_ = h; }
    constexpr void await_resume() const noexcept {}
};

ReturnObject counter() {
    int i = 0;
    while (1) {
        co_await i;
        i++;
    }
}
int main() {
    ReturnObject ret = counter();
    for (int i = 0; i < 5; ++i) {
        std::cout << ret.next() << '\n';
    }
    return 0;
}