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

PythonBytes ZMQClient::request_latest(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_LATEST_DATA, {});
    return send_message_(message);
}

std::vector<PythonBytes> ZMQClient::request_all(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_ALL_DATA, {});
    PythonBytes reply = send_message_(message);
    return deserialize_multiple_data_(reply);
}

std::vector<PythonBytes> ZMQClient::request_last_k(const std::string &topic, int k)
{
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, PythonBytes(k, 0));
    PythonBytes reply = send_message_(message);
    return deserialize_multiple_data_(reply);
}

PythonBytes ZMQClient::request_with_data(const std::string &topic, const PythonBytes &data)
{
    ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, data);
    return send_message_(message);
}

std::vector<PythonBytes> ZMQClient::deserialize_multiple_data_(const PythonBytes &data)
{
    // convert the first 2 bytes to a short
    short num_data = *reinterpret_cast<const short *>(data.data());
    // the next num_data * sizeof(int) bytes are the lengths of the packages
    std::vector<int> package_lengths(num_data);
    std::memcpy(package_lengths.data(), data.data() + 2, num_data * sizeof(int));
    // the rest of the bytes are the data
    std::vector<PythonBytes> result(num_data);
    int offset = 2 + num_data * sizeof(int);
    for (int i = 0; i < num_data; ++i)
    {
        result[i] = PythonBytes(data.data() + offset, data.data() + offset + package_lengths[i]);
        offset += package_lengths[i];
    }
    return result;
}

PythonBytes ZMQClient::send_message_(const ZMQMessage &message)
{
    PythonBytes serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    zmq::message_t reply;
    socket_.recv(reply);
    ZMQMessage reply_message(PythonBytes(reply.data<char>(), reply.data<char>() + reply.size()));
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
