#include "zmq_message.h"

ZMQMessage::ZMQMessage(const std::string &topic, CmdType cmd, const PyBytesPtr data_ptr) : topic_(topic), cmd_(cmd)
{
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
    if (data_ptr == nullptr)
    {
        throw std::invalid_argument("Data cannot be null");
    }
    data_str_ = *data_ptr;
}

ZMQMessage::ZMQMessage(const std::string &topic, CmdType cmd, const std::string &data_str) : topic_(topic), cmd_(cmd)
{
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
    data_str_ = data_str;
}

ZMQMessage::ZMQMessage(const std::string &serialized)
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
    data_str_ = std::string(serialized.begin() + 2 + topic_length, serialized.end());
}

std::string ZMQMessage::topic() const
{
    return topic_;
}

CmdType ZMQMessage::cmd() const
{
    return cmd_;
}

PyBytesPtr ZMQMessage::data_ptr() const
{
    return std::make_shared<pybind11::bytes>(data_str_);
}

std::string ZMQMessage::data_str() const
{
    return data_str_;
}

std::string ZMQMessage::serialize() const
{
    std::string serialized;
    serialized.push_back(topic_.size());
    serialized.insert(serialized.end(), topic_.begin(), topic_.end());
    serialized.push_back(static_cast<char>(cmd_));
    serialized.insert(serialized.end(), data_str_.begin(), data_str_.end());
    return serialized;
}

ZMQMultiPtrMessage::ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::vector<PyBytesPtr> &data_ptrs)
    : topic_(topic), cmd_(cmd), data_ptrs_(data_ptrs)
{
    check_input_validity_();
    data_str_ = encode_data_blocks(data_ptrs);
}

ZMQMultiPtrMessage::ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::string &data_str)
    : topic_(topic), cmd_(cmd), data_str_(data_str)
{
    check_input_validity_();
}

ZMQMultiPtrMessage::ZMQMultiPtrMessage(const std::string &serialized)
{
    short topic_length = serialized[0];
    if (serialized.size() < 1 + topic_length + 1)
    {
        throw std::invalid_argument("Serialized message is too short");
    }
    topic_ = std::string(serialized.begin() + 1, serialized.begin() + 1 + topic_length);
    cmd_ = static_cast<CmdType>(serialized[1 + topic_length]);
    data_str_ = std::string(serialized.begin() + 2 + topic_length, serialized.end());
    data_ptrs_ = decode_data_blocks(data_str_);
}

std::string ZMQMultiPtrMessage::topic() const
{
    return topic_;
}

CmdType ZMQMultiPtrMessage::cmd() const
{
    return cmd_;
}

std::vector<PyBytesPtr> ZMQMultiPtrMessage::data_ptrs()
{
    if (data_ptrs_.empty())
    {
        if (data_str_.empty())
        {
            throw std::runtime_error("Data is empty");
        }
        data_ptrs_ = decode_data_blocks(data_str_);
    }
    return data_ptrs_;
}

std::string ZMQMultiPtrMessage::encode_data_blocks(const std::vector<PyBytesPtr> &data_ptrs)
{
    int data_string_length = sizeof(int); // block_num
    int block_num = data_ptrs.size();
    std::vector<int> data_lengths;
    for (const auto &data_ptr : data_ptrs)
    {
        int data_length = sizeof(data_ptr);
        data_string_length += data_length + sizeof(int);
        data_lengths.push_back(data_length);
    }
    std::string data_str;
    data_str.reserve(data_string_length);
    data_str.push_back(block_num);
    for (int i = 0; i < block_num; ++i)
    {
        data_str.push_back(data_lengths[i]);
    }
    int data_start_index = 1 + block_num;
    for (const auto &data_ptr : data_ptrs)
    {
        std::string data_block_str = *data_ptr;
        data_str.append(data_block_str);
    }
    assert(data_str.size() == data_string_length);
    return data_str;
}

std::vector<PyBytesPtr> ZMQMultiPtrMessage::decode_data_blocks(const std::string &data_str)
{
    std::vector<PyBytesPtr> data_ptrs;
    int block_num = data_str[0];
    int data_start_index = 1 + block_num;
    for (int i = 0; i < block_num; ++i)
    {
        int data_length = data_str[i + 1];
        data_ptrs.push_back(
            std::make_shared<pybind11::bytes>(data_str.substr(data_start_index, data_start_index + data_length)));
        data_start_index += data_length;
    }
    return data_ptrs;
}

void ZMQMultiPtrMessage::check_input_validity_()
{
    // Check whether CMD is a valid value in the enum CmdType
    if (cmd_ != CmdType::GET_ALL_DATA && cmd_ != CmdType::GET_LAST_K_DATA && cmd_ != CmdType::ERROR)
    {
        throw std::invalid_argument(
            "ZMQMultiPtrMessage only supports GET_ALL_DATA, GET_LAST_K_DATA, ERROR command types");
    }
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
    for (const auto &data_ptr : data_ptrs_)
    {
        if (data_ptr == nullptr)
        {
            throw std::invalid_argument("Data cannot be null");
        }
    }
}
