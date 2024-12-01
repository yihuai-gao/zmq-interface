#pragma once

#include "common.h"
#include <pybind11/pybind11.h>
#include <vector>
#include <zmq.hpp>
enum class CmdType : int
{
    // Other positive values: to request corresponding number of data
    GET_LATEST_DATA = 1,
    GET_ALL_DATA = 2,
    GET_LAST_K_DATA = 3,
    REQUEST_WITH_DATA = 4,
    ERROR = -1,
    UNKNOWN = 0,
};

class ZMQMessage
{
  public:
    ZMQMessage(const std::string &topic, CmdType cmd, const PyBytesPtr data_ptr);
    ZMQMessage(const std::string &topic, CmdType cmd, const std::string &data_str);
    ZMQMessage(const std::string &serialized);
    std::string topic() const;
    CmdType cmd() const;
    PyBytesPtr data_ptr() const;
    std::string data_str() const; // Should avoid using it if data has a large size
    std::string serialize() const;

  private:
    std::string topic_;
    CmdType cmd_;
    std::string data_;
};
