#pragma once
#include <chrono>
#include <memory>
#include <pybind11/pybind11.h>
#include <string>
#include <vector>
#define get_time_us()                                                                                                  \
    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()

// using PythonBytesPtr = std::shared_ptr<std::string>;
using PythonBytesPtr = std::shared_ptr<pybind11::bytes>;