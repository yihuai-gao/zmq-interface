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
    return *send_message_(message)[0];
}

std::vector<PyBytes> ZMQClient::request_all(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_ALL_DATA, std::string());
    std::vector<PyBytesPtr> replied_pointers = send_message_(message);
    std::vector<PyBytes> reply;
    for (const PyBytesPtr ptr : replied_pointers)
    {
        reply.push_back(*ptr);
    }
    return reply;
}

std::vector<PyBytes> ZMQClient::request_last_k(const std::string &topic, int k)
{
    std::string data_str(reinterpret_cast<const char *>(&k), sizeof(int));
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, data_str);
    std::vector<PyBytesPtr> replied_pointers = send_message_(message);
    std::vector<PyBytes> reply;
    for (const PyBytesPtr ptr : replied_pointers)
    {
        reply.push_back(*ptr);
    }
    return reply;
}

PyBytes ZMQClient::request_with_data(const std::string &topic, const PyBytes data)
{
    PyBytesPtr data_ptr = std::make_shared<pybind11::bytes>(data);
    ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, data_ptr);
    return *send_message_(message)[0];
}

std::vector<PyBytesPtr> ZMQClient::send_message_(const ZMQMessage &message)
{
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    std::vector<PyBytesPtr> replies;
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
    if (reply_message.cmd() == CmdType::GET_LATEST_DATA || reply_message.cmd() == CmdType::REQUEST_WITH_DATA)
    {
        replies.push_back(reply_message.data_ptr());
        return replies;
    }
    if (reply_message.cmd() == CmdType::GET_ALL_DATA || reply_message.cmd() == CmdType::GET_LAST_K_DATA)
    {
        printf("reply_message.data_str().length() = %d, lenreply_message.data_str().data() = %s\n",
               reply_message.data_str().length(), reply_message.data_str().data());
        int num_data = *reinterpret_cast<const int *>(reply_message.data_str().data());
        printf("num_data = %d\n", num_data);
        for (int i = 0; i < num_data; i++)
        {
            zmq::message_t reply_data;
            socket_.recv(reply_data);
            ZMQMessage reply(std::string(reply_data.data<char>(), reply_data.data<char>() + reply_data.size()));
            replies.push_back(reply.data_ptr());
            printf("Pushed back reply.data_ptr(): %d\n", i);
        }
    }
    return replies;
}
