#include "Scheduler.hpp"
#include <thread>

size_t Scheduler::thread_ct_ = 5;
volatile bool Scheduler::running_ = false;
std::mutex Scheduler::m_;
std::vector<std::thread> Scheduler::threads_;
MultiThreadQueue<std::coroutine_handle<>> Scheduler::queue_;
std::thread Scheduler::t_;