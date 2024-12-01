#include "zmq_client.h"

ZMQClient::ZMQClient(const std::string &server_endpoint) : context_(1), socket_(context_, zmq::socket_type::req)
{
    socket_.connect(server_endpoint);
}

ZMQClient::~ZMQClient()
{
    socket_.close();
    context_.close();
}

PyBytes ZMQClient::request_latest(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_LATEST_DATA, std::string());
    return *send_single_block_request_(message);
}

std::vector<PyBytes> ZMQClient::request_all(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_ALL_DATA, std::string());
    std::vector<PyBytesPtr> request_ptrs = send_multi_block_request_(message);
    std::vector<PyBytes> reply;
    for (const PyBytesPtr ptr : request_ptrs)
    {
        reply.push_back(*ptr);
    }
    return reply;
}

std::vector<PyBytes> ZMQClient::request_last_k(const std::string &topic, int k)
{
    std::string data_str(reinterpret_cast<const char *>(&k), sizeof(int));
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, data_str);
    std::vector<PyBytesPtr> request_ptrs = send_multi_block_request_(message);
    std::vector<PyBytes> reply;
    for (const PyBytesPtr ptr : request_ptrs)
    {
        reply.push_back(*ptr);
    }
    return reply;
}

PyBytes ZMQClient::request_with_data(const std::string &topic, const PyBytes data)
{
    PyBytesPtr data_ptr = std::make_shared<pybind11::bytes>(data);
    ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, data_ptr);
    return *send_single_block_request_(message);
}

std::vector<PyBytes> ZMQClient::get_last_retrieved_data()
{
    if (last_retrieved_ptrs_.empty())
    {
        return {};
    }
    std::vector<PyBytes> data;
    for (const PyBytesPtr ptr : last_retrieved_ptrs_)
    {
        data.push_back(*ptr);
    }
    return data;
}

PyBytesPtr ZMQClient::send_single_block_request_(const ZMQMessage &message)
{
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    zmq::message_t reply;
    socket_.recv(reply);
    // printf("Receiving message with data: %s\n",
    //        bytes_to_hex(std::string(reply.data<char>(), reply.data<char>() + reply.size())).c_str());
    ZMQMessage reply_message(std::string(reply.data<char>(), reply.data<char>() + reply.size()));
    if (reply_message.cmd() == CmdType::ERROR)
    {
        throw std::runtime_error("Server returned error: " + reply_message.data_str());
    }
    if (reply_message.cmd() != message.cmd())
    {
        throw std::runtime_error("Command type mismatch. Sent " + std::to_string(static_cast<int>(message.cmd())) +
                                 " but received " + std::to_string(static_cast<int>(reply_message.cmd())));
    }
    if (reply_message.cmd() == CmdType::GET_LATEST_DATA || reply_message.cmd() == CmdType::REQUEST_WITH_DATA)
    {
        last_retrieved_ptrs_ = {reply_message.data_ptr()};
        return reply_message.data_ptr();
    }
    throw std::runtime_error("Invalid command type: " + std::to_string(static_cast<int>(reply_message.cmd())));
}

std::vector<PyBytesPtr> ZMQClient::send_multi_block_request_(const ZMQMessage &message)
{
    std::vector<PyBytesPtr> reply_ptrs;
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    zmq::message_t reply;
    socket_.recv(reply);
    ZMQMultiPtrMessage reply_message(std::string(reply.data<char>(), reply.data<char>() + reply.size()));
    if (reply_message.cmd() == CmdType::ERROR)
    {
        throw std::runtime_error("Server returned error: " + reply_message.data_str());
    }
    if (reply_message.cmd() != message.cmd())
    {
        throw std::runtime_error("Command type mismatch. Sent " + std::to_string(static_cast<int>(message.cmd())) +
                                 " but received " + std::to_string(static_cast<int>(reply_message.cmd())));
    }
    if (reply_message.cmd() == CmdType::GET_ALL_DATA || reply_message.cmd() == CmdType::GET_LAST_K_DATA)
    {
        last_retrieved_ptrs_ = reply_message.data_ptrs();
        return reply_message.data_ptrs();
    }
    throw std::runtime_error("Invalid command type: " + std::to_string(static_cast<int>(reply_message.cmd())));
}
