#pragma once
#include <chrono>
#include <string>
#define get_time_us()                                                                                                  \
    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()

using PythonBytes = std::string;