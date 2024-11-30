#include "zmq_message.h"

ZMQMessage::ZMQMessage(const std::string &topic, CmdType cmd, const std::vector<char> &data)
    : topic_(topic), cmd_(cmd), data_(data)
{
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
}

ZMQMessage::ZMQMessage(const std::vector<char> &serialized)
{
    if (serialized.size() < 3)
    {
        throw std::invalid_argument("Serialized message must be at least 3 bytes");
    }
    short topic_length = serialized[0];
    if (serialized.size() < 1 + topic_length + 1)
    {
        throw std::invalid_argument("Serialized message is too short");
    }
    topic_ = std::string(serialized.begin() + 1, serialized.begin() + 1 + topic_length);
    cmd_ = static_cast<CmdType>(serialized[1 + topic_length]);
    data_ = std::vector<char>(serialized.begin() + 2 + topic_length, serialized.end());
}

std::string ZMQMessage::topic() const
{
    return topic_;
}

CmdType ZMQMessage::cmd() const
{
    return cmd_;
}

std::vector<char> ZMQMessage::data() const
{
    return data_;
}

std::vector<char> ZMQMessage::serialize() const
{
    std::vector<char> serialized;
    serialized.push_back(topic_.size());
    serialized.insert(serialized.end(), topic_.begin(), topic_.end());
    serialized.push_back(static_cast<char>(cmd_));
    serialized.insert(serialized.end(), data_.begin(), data_.end());
    return serialized;
}
