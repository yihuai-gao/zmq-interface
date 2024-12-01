#include "zmq_client.h"

ZMQClient::ZMQClient(const std::string &client_name, const std::string &server_endpoint)
    : context_(1), socket_(context_, zmq::socket_type::req), steady_clock_start_time_us_(steady_clock_us()),
      last_retrieved_ptrs_(), logger_(spdlog::stdout_color_mt(client_name))
{
    socket_.connect(server_endpoint);
}

ZMQClient::~ZMQClient()
{
    socket_.close();
    context_.close();
}

pybind11::list ZMQClient::request_latest(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_LATEST_DATA, std::string(), get_timestamp());
    TimedPtr ptr = send_single_block_request_(message);
    pybind11::list reply;
    reply.append(*std::get<0>(ptr));
    reply.append(std::get<1>(ptr));
    return reply;
}

pybind11::list ZMQClient::request_all(const std::string &topic)
{
    ZMQMessage message(topic, CmdType::GET_ALL_DATA, std::string(), get_timestamp());
    std::vector<TimedPtr> request_ptrs = send_multi_block_request_(message);
    pybind11::list reply;
    for (const TimedPtr ptr : request_ptrs)
    {
        pybind11::list single_reply;
        single_reply.append(*std::get<0>(ptr));
        single_reply.append(std::get<1>(ptr));
        reply.append(single_reply);
    }
    return reply;
}

pybind11::list ZMQClient::request_last_k(const std::string &topic, uint32_t k)
{
    std::string data_str = uint32_to_bytes(k);
    ZMQMessage message(topic, CmdType::GET_LAST_K_DATA, data_str, get_timestamp());
    std::vector<TimedPtr> request_ptrs = send_multi_block_request_(message);
    pybind11::list reply;
    for (const TimedPtr ptr : request_ptrs)
    {
        pybind11::list single_reply;
        single_reply.append(*std::get<0>(ptr));
        single_reply.append(std::get<1>(ptr));
        reply.append(single_reply);
    }
    return reply;
}

// PyBytes ZMQClient::request_with_data(const std::string &topic, const PyBytes data)
// {
//     TimedPtr data_ptr = std::make_shared<pybind11::bytes>(data);
//     ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, std::make_tuple(data_ptr, get_timestamp()));
//     return *send_single_block_request_(message);
// }

pybind11::list ZMQClient::get_last_retrieved_data()
{
    if (last_retrieved_ptrs_.empty())
    {
        return pybind11::list();
    }
    pybind11::list data;
    for (const TimedPtr ptr : last_retrieved_ptrs_)
    {
        pybind11::list single_reply;
        single_reply.append(*std::get<0>(ptr));
        single_reply.append(std::get<1>(ptr));
        data.append(single_reply);
    }
    return data;
}

double ZMQClient::get_timestamp()
{
    return static_cast<double>(steady_clock_us() - steady_clock_start_time_us_) / 1e6;
}

void ZMQClient::reset_start_time(int64_t system_time_us)
{
    steady_clock_start_time_us_ = steady_clock_us() + (system_time_us - system_clock_us());
}

TimedPtr ZMQClient::send_single_block_request_(const ZMQMessage &message)
{
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
    zmq::message_t reply;
    socket_.recv(reply);
    ZMQMessage reply_message(std::string(reply.data<char>(), reply.data<char>() + reply.size()), get_timestamp());
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

std::vector<TimedPtr> ZMQClient::send_multi_block_request_(const ZMQMessage &message)
{
    std::vector<TimedPtr> reply_ptrs;
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
