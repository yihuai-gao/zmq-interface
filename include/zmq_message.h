#pragma once

#include <zmq.hpp>

#include <vector>

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
    ZMQMessage(const std::string &topic, CmdType cmd, const std::vector<char> &data);
    ZMQMessage(const std::vector<char> &serialized);
    std::string topic() const;
    CmdType cmd() const;
    std::vector<char> data() const;
    std::vector<char> serialize() const;

  private:
    std::string topic_;
    CmdType cmd_;
    std::vector<char> data_;
};
