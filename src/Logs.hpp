#pragma once

#include "Scheduler.hpp"
#include "Task.hpp"
#include <ios>
#include <iostream>
#include <stdarg.h>
#include <string>
#include <thread>
#define LOG(...) __debug__(std::this_thread::get_id(), __LINE__, __VA_ARGS__);
static Scheduler __debug_s__(1);
template <class... Args>
Task<void> __debug__(std::thread::id id, size_t line, Args... args) {
    // std::cerr << __func__ << std::endl;
    co_await __debug_s__;
    std::cerr << "[" << std::hex << id << std::dec << "] ";
    std::cerr << "at line: " << line << ": ";
    (std::cerr << ... << args) << std::endl;
}