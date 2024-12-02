#pragma once
#include <chrono>
#include <memory>
#include <pybind11/pybind11.h>
#include <string>
#include <tuple>
#include <vector>

#include <iomanip>
#include <sstream>
using PyBytes = pybind11::bytes;
using PyBytesPtr = std::shared_ptr<pybind11::bytes>;

using TimedPtr = std::tuple<PyBytesPtr, double>;
int64_t steady_clock_us();
int64_t system_clock_us();

enum class EndType : int8_t
{
    NONE = 0,
    EARLIEST = 1,
    LATEST = 2,
};

std::string uint32_to_bytes(uint32_t value);
uint32_t bytes_to_uint32(const std::string &bytes);
std::string int32_to_bytes(int32_t value);
int32_t bytes_to_int32(const std::string &bytes);
std::string double_to_bytes(double value);
double bytes_to_double(const std::string &bytes);
std::string bytes_to_hex(const std::string &bytes);
std::string end_type_to_str(EndType end_type);
EndType str_to_end_type(const std::string &end_type);
