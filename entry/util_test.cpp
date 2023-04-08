#include "Scheduler.hpp"
#include "TCPServer.hpp"
#include "Task.hpp"
#include <algorithm>
#include <assert.h>
#include <condition_variable>
#include <coroutine>
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

Task<int> async_call() {
    DEBUG_PRINT("#1 in async call. this should not block");
    sleep(2);
    DEBUG_PRINT("#3 in async call. end");

    co_yield 101;
}

Task<TCPServer::Response> test() {
    sleep(1);
    DEBUG_PRINT("?????????");
    co_yield TCPServer::Response();
}

int main() {
    DEBUG_PRINT("main");
    Scheduler::start();

    // sleep(4);
    // t.then([](int res) { std::cout << "result: " << res << '\n'; });
    auto reading = []() -> Task<void> {
        char buffer[1024];
        DEBUG_PRINT("start reading");
        std::string result = "This is a test";
        co_await test();
        DEBUG_PRINT("END OF CO_AWAIT");
    }();
    Scheduler::join();
}
