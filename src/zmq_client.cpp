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

std::vector<char> ZMQClient::request_latest(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_LATEST_DATA, {});
    return send_message_(message);
}

std::vector<std::vector<char>> ZMQClient::request_last_k(const std::string &topic, int k)
{
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, std::vector<char>(k));
    std::vector<char> reply = send_message_(message);
    // convert the first 2 bytes to a short
    short num_data = *reinterpret_cast<short *>(reply.data()); // this might be smaller than k
    // the next num_data * sizeof(int) bytes are the lengths of the packages
    std::vector<int> package_lengths(num_data);
    std::memcpy(package_lengths.data(), reply.data() + 2, num_data * sizeof(int));
    // the rest of the bytes are the data
    std::vector<std::vector<char>> data(num_data);
    int offset = 2 + num_data * sizeof(int);
    for (int i = 0; i < num_data; ++i)
    {
        data[i] = std::vector<char>(reply.data() + offset, reply.data() + offset + package_lengths[i]);
        offset += package_lengths[i];
    }
    return data;
}

std::vector<char> ZMQClient::request_with_data(const std::string &topic, const std::vector<char> &data)
{
    ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, data);
    return send_message_(message);
}

std::vector<char> ZMQClient::send_message_(const ZMQMessage &message)
{
    std::vector<char> serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    zmq::message_t reply;
    socket_.recv(reply);
    ZMQMessage reply_message(std::vector<char>(reply.data<char>(), reply.data<char>() + reply.size()));
    if (reply_message.cmd() == CmdType::ERROR)
    {
        throw std::runtime_error("Server returned error: " +
                                 std::string(reply_message.data().begin(), reply_message.data().end()));
    }
    if (reply_message.cmd() != message.cmd())
    {
        throw std::runtime_error("Command type mismatch. Sent " + std::to_string(static_cast<int>(message.cmd())) +
                                 " but received " + std::to_string(static_cast<int>(reply_message.cmd())));
    }
    return reply_message.data();
}
