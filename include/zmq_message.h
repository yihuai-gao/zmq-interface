#pragma once

#include "common.h"
#include <pybind11/pybind11.h>
#include <vector>
#include <zmq.hpp>
enum class CmdType : int8_t
{
    GET_LATEST_DATA = 1,
    GET_ALL_DATA = 2,
    GET_LAST_K_DATA = 3,
    REQUEST_WITH_DATA = 4,
    SYNCHRONIZE_TIME = 5,
    ERROR = -1,
    UNKNOWN = 0,
};

class ZMQMessage
{
  public:
    ZMQMessage(const std::string &topic, CmdType cmd, const TimedPtr data_ptr);
    ZMQMessage(const std::string &topic, CmdType cmd, const std::string &data_str, double timestamp);
    ZMQMessage(const std::string &serialized, double timestamp);
    std::string topic() const;
    CmdType cmd() const;
    TimedPtr data_ptr() const;
    std::string data_str() const; // Should avoid using because it may copy a large amount of data
    std::string serialize() const;

  private:
    int data_size_;
    std::string topic_;
    CmdType cmd_;
    double timestamp_;
    TimedPtr data_ptr_;
    std::string data_str_;
};

class ZMQMultiPtrMessage
{
  public:
    ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::vector<TimedPtr> &data_ptrs);
    ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::string &data_str);
    ZMQMultiPtrMessage(const std::string &serialized);

    std::string topic() const;
    CmdType cmd() const;
    std::vector<TimedPtr> data_ptrs();
    std::string data_str(); // Should avoid using because it may copy a large amount of data
    std::string serialize();

  private:
    void encode_data_blocks_();
    void decode_data_blocks_();
    void check_input_validity_();
    std::string topic_;
    CmdType cmd_;
    std::vector<TimedPtr> data_ptrs_;
    std::string data_str_;
};
