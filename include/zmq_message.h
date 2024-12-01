#pragma once

#include "common.h"
#include <pybind11/pybind11.h>
#include <vector>
#include <zmq.hpp>
enum class CmdType : int
{
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
    std::string data_str() const; // Should avoid using because it may copy a large amount of data
    std::string serialize() const;

  private:
    int data_size_;
    std::string topic_;
    CmdType cmd_;
    std::string data_str_;
};

class ZMQMultiPtrMessage
{
  public:
    ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::vector<PyBytesPtr> &data_ptrs);
    ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::string &data_str);
    ZMQMultiPtrMessage(const std::string &serialized);

    std::string topic() const;
    CmdType cmd() const;
    std::vector<PyBytesPtr> data_ptrs();
    std::string data_str() const; // Should avoid using because it may copy a large amount of data
    std::string serialize() const;
    static std::string encode_data_blocks(const std::vector<PyBytesPtr> &data_ptrs);
    static std::vector<PyBytesPtr> decode_data_blocks(const std::string &data_str);

  private:
    void check_input_validity_();
    std::string topic_;
    CmdType cmd_;
    std::vector<PyBytesPtr> data_ptrs_;
    std::string data_str_;
};
