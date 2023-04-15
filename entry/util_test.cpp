#include "Scheduler.hpp"
#include "TCPServer.hpp"
#include "Task.hpp"
#include <algorithm>
#include <assert.h>
#include <condition_variable>
#include <coroutine>
#include <csignal>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <optional>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

Task<int> async_call(int i) {
    DEBUG_PRINT("#1 in async call. this should not block");
    sleep(2);
    DEBUG_PRINT("#3 in async call. end");

    co_return 101;
}

Task<TCPServer::Response> test(int &i) {
    sleep(1);
    DEBUG_PRINT("?????????");
    co_return TCPServer::Response();
}

int main() {
    DEBUG_PRINT("main");
    Scheduler s(100);
    std::signal(SIGINT, Scheduler::SIGINT_HANDLER);
    // sleep(4);
    // t.then([](int res) { std::cout << "result: " << res << '\n'; });
    auto reading = [&s]() -> Task<void> {
        DEBUG_PRINT("before reading");
        co_await s;
        DEBUG_PRINT("start reading");
        co_await async_call(5);
        DEBUG_PRINT("END OF CO_AWAIT");
    }();
}
