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

PythonBytesPtr ZMQClient::request_latest(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_LATEST_DATA, std::string());
    return send_message_(message)[0];
}

std::vector<PythonBytesPtr> ZMQClient::request_all(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_ALL_DATA, std::string());
    std::vector<PythonBytesPtr> reply = send_message_(message);
    return reply;
}

std::vector<PythonBytesPtr> ZMQClient::request_last_k(const std::string &topic, int k)
{
    std::string data_str(reinterpret_cast<const char *>(&k), sizeof(int));
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, data_str);
    std::vector<PythonBytesPtr> reply = send_message_(message);
    return reply;
}

PythonBytesPtr ZMQClient::request_with_data(const std::string &topic, const PythonBytesPtr data_ptr)
{
    ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, data_ptr);
    return send_message_(message)[0];
}

std::vector<PythonBytesPtr> ZMQClient::send_message_(const ZMQMessage &message)
{
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    std::vector<PythonBytesPtr> replies;
    zmq::message_t reply;
    socket_.recv(reply);
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
    replies.push_back(reply_message.data_ptr());
    return replies;
}
