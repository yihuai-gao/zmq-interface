#include "zmq_message.h"
ZMQMessage::ZMQMessage(const std::string &topic, CmdType cmd, const TimedPtr data_ptr) : topic_(topic), cmd_(cmd)
{
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
    if (std::get<0>(data_ptr) == nullptr)
    {
        throw std::invalid_argument("Data cannot be null");
    }
    data_str_ = *std::get<0>(data_ptr);
}

ZMQMessage::ZMQMessage(const std::string &topic, CmdType cmd, const std::string &data_str, double timestamp)
    : data_str_(data_str), topic_(topic), cmd_(cmd), timestamp_(timestamp)
{
    if (topic_.size() > 255)
    {
        throw std::invalid_argument("Topic size must be less than 256 characters");
    }
    if (topic_.empty())
    {
        throw std::invalid_argument("Topic cannot be empty");
    }
    data_ptr_ = std::make_tuple(std::make_shared<PyBytes>(PyBytes(data_str_)), timestamp_);
}

ZMQMessage::ZMQMessage(const std::string &serialized)
{
    if (serialized.size() < 3)
    {
        throw std::invalid_argument("Serialized message must be at least 3 bytes");
    }
    uint8_t topic_length = static_cast<uint8_t>(serialized[0]);
    if (serialized.size() < sizeof(uint8_t) + topic_length + 1)
    {
        throw std::invalid_argument("Serialized message is too short");
    }
    topic_ = std::string(serialized.begin() + sizeof(uint8_t), serialized.begin() + sizeof(uint8_t) + topic_length);
    cmd_ = static_cast<CmdType>(serialized[sizeof(uint8_t) + topic_length]);
    data_str_ = std::string(serialized.begin() + 2 * sizeof(uint8_t) + topic_length, serialized.end());
}

std::string ZMQMessage::topic() const
{
    return topic_;
}

CmdType ZMQMessage::cmd() const
{
    return cmd_;
}

TimedPtr ZMQMessage::data_ptr() const
{

    return data_ptr_;
}

std::string ZMQMessage::data_str() const
{
    return data_str_;
}

std::string ZMQMessage::serialize() const
{
    std::string serialized;
    serialized.push_back(static_cast<char>(uint8_t(topic_.size())));
    serialized.append(topic_);
    serialized.push_back(static_cast<char>(cmd_));
    serialized.append(data_str_);
    // printf("serailized: %s\n", bytes_to_hex(serialized).c_str());
    return serialized;
}

ZMQMultiPtrMessage::ZMQMultiPtrMessage(const std::string &topic, CmdType cmd, const std::vector<TimedPtr> &data_ptrs)
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
    uint8_t topic_length = static_cast<uint8_t>(serialized[0]);
    if (serialized.size() < sizeof(uint8_t) + topic_length + sizeof(uint8_t))
    {
        throw std::invalid_argument("Serialized message is too short");
    }
    topic_ = std::string(serialized.begin() + sizeof(uint8_t), serialized.begin() + sizeof(uint8_t) + topic_length);
    cmd_ = static_cast<CmdType>(serialized[sizeof(int8_t) + topic_length]);
    data_str_ = std::string(serialized.begin() + 2 * sizeof(uint8_t) + topic_length, serialized.end());
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

std::vector<TimedPtr> ZMQMultiPtrMessage::data_ptrs()
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

std::string ZMQMultiPtrMessage::data_str() const
{
    return data_str_;
}

std::string ZMQMultiPtrMessage::serialize() const
{
    std::string serialized;
    serialized.push_back(static_cast<char>(uint8_t(topic_.size())));
    serialized.append(topic_);
    serialized.push_back(static_cast<char>(cmd_));
    serialized.append(data_str_);
    return serialized;
}

std::string ZMQMultiPtrMessage::encode_data_blocks(const std::vector<TimedPtr> &data_ptrs)
{
    uint32_t data_string_length = sizeof(uint32_t); // block_num
    uint32_t block_num = data_ptrs.size();
    std::vector<uint32_t> data_lengths;
    std::vector<double> timestamps;
    for (const auto &data_ptr : data_ptrs)
    {
        int data_length = pybind11::len(*std::get<0>(data_ptr));
        data_string_length += data_length + sizeof(uint32_t) + sizeof(double);
        data_lengths.push_back(data_length);
        timestamps.push_back(std::get<1>(data_ptr));
    }
    std::string data_str;
    data_str.reserve(data_string_length);
    data_str.append(uint32_to_bytes(block_num));
    for (int i = 0; i < block_num; ++i)
    {
        data_str.append(uint32_to_bytes(data_lengths[i]));
        data_str.append(double_to_bytes(timestamps[i]));
    }
    int data_start_index = 1 + block_num;
    for (const auto &data_ptr : data_ptrs)
    {
        std::string data_block_str = *std::get<0>(data_ptr);
        data_str.append(data_block_str);
    }
    assert(data_str.size() == data_string_length);
    // printf("encode_data_blocks: %s\n", bytes_to_hex(data_str).c_str());
    return data_str;
}

std::vector<TimedPtr> ZMQMultiPtrMessage::decode_data_blocks(const std::string &data_str)
{
    std::vector<TimedPtr> data_ptrs;
    uint32_t block_num = bytes_to_uint32(std::string(data_str.data(), sizeof(uint32_t)));
    int index_size = sizeof(uint32_t) + sizeof(double);
    int data_start_index = (1 + block_num) * sizeof(uint32_t);
    // printf("decode_data_blocks: %s\n", bytes_to_hex(data_str).c_str());
    for (int i = 0; i < block_num; ++i)
    {
        std::string data_length_str(data_str.data() + sizeof(uint32_t) + i * index_size, sizeof(uint32_t));

        uint32_t data_length = bytes_to_uint32(data_length_str);
        if (data_start_index + data_length > data_str.size())
        {
            throw std::invalid_argument("Data block length invalid. Please check the data string");
        }
        if (data_length < 0)
        {
            throw std::invalid_argument("Data block length must be non-negative");
        }
        std::string data_timestamp_str(data_str.data() + 2 * sizeof(uint32_t) + i * index_size, sizeof(double));
        double timestamp = bytes_to_double(data_timestamp_str);
        if (data_length == 0)
        {
            data_ptrs.push_back(std::make_tuple(std::make_shared<PyBytes>(PyBytes("")), timestamp));
            continue;
        }
        data_ptrs.push_back(std::make_tuple(
            std::make_shared<PyBytes>(PyBytes(data_str.data() + data_start_index, data_length)), timestamp));
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
        if (std::get<0>(data_ptr) == nullptr)
        {
            throw std::invalid_argument("Data cannot be null");
        }
    }
}
