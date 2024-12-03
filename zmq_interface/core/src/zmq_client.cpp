#include "zmq_client.h"

ZMQClient::ZMQClient(const std::string &client_name, const std::string &server_endpoint)
    : context_(1), socket_(context_, zmq::socket_type::req), steady_clock_start_time_us_(steady_clock_us()),
      last_retrieved_ptrs_(), logger_(spdlog::stdout_color_mt(client_name))
{
    logger_->set_pattern("[%H:%M:%S %n %^%l%$] %v");
    socket_.connect(server_endpoint);
}

ZMQClient::~ZMQClient()
{
    socket_.close();
    context_.close();
}

pybind11::tuple ZMQClient::peek_data(const std::string &topic, std::string end_type, int32_t n)
{
    std::string data_str = int32_to_bytes(n);
    ZMQMessage message(topic, CmdType::PEEK_DATA, str_to_end_type(end_type), get_timestamp(), data_str);
    std::vector<TimedPtr> reply_ptrs = send_request_(message);
    pybind11::list data;
    pybind11::list timestamps;
    if (reply_ptrs.empty())
    {
        logger_->debug("No data available for topic: {}", topic);
    }
    for (const TimedPtr ptr : reply_ptrs)
    {
        data.append(*std::get<0>(ptr));
        timestamps.append(std::get<1>(ptr));
    }
    return pybind11::make_tuple(data, timestamps);
}

pybind11::tuple ZMQClient::pop_data(const std::string &topic, std::string end_type, int32_t n)
{
    std::string data_str = int32_to_bytes(n);
    ZMQMessage message(topic, CmdType::POP_DATA, str_to_end_type(end_type), get_timestamp(), data_str);
    std::vector<TimedPtr> reply_ptrs = send_request_(message);
    pybind11::list data;
    pybind11::list timestamps;
    if (reply_ptrs.empty())
    {
        logger_->debug("No data available for topic: {}", topic);
    }
    for (const TimedPtr ptr : reply_ptrs)
    {
        data.append(*std::get<0>(ptr));
        timestamps.append(std::get<1>(ptr));
    }
    return pybind11::make_tuple(data, timestamps);
}

// PyBytes ZMQClient::request_with_data(const std::string &topic, const PyBytes data)
// {
//     TimedPtr data_ptr = std::make_shared<pybind11::bytes>(data);
//     ZMQMessage message(topic, CmdType::REQUEST_WITH_DATA, std::make_tuple(data_ptr, get_timestamp()));
//     return *send_single_block_request_(message);
// }

pybind11::tuple ZMQClient::get_last_retrieved_data()
{
    if (last_retrieved_ptrs_.empty())
    {
        return pybind11::make_tuple(pybind11::list(), pybind11::list());
    }
    pybind11::list data;
    pybind11::list timestamps;
    for (const TimedPtr ptr : last_retrieved_ptrs_)
    {
        data.append(*std::get<0>(ptr));
        timestamps.append(std::get<1>(ptr));
    }
    return pybind11::make_tuple(data, timestamps);
}

double ZMQClient::get_timestamp()
{
    return static_cast<double>(steady_clock_us() - steady_clock_start_time_us_) / 1e6;
}

void ZMQClient::reset_start_time(int64_t system_time_us)
{
    logger_->info("Resetting start time. Will clear all data retrieved before this time");
    last_retrieved_ptrs_.clear();
    steady_clock_start_time_us_ = steady_clock_us() + (system_time_us - system_clock_us());
}

// TimedPtr ZMQClient::send_single_block_request_(const ZMQMessage &message)
// {
//     std::string serialized = message.serialize();
//     zmq::message_t request(serialized.data(), serialized.size());
//     socket_.send(request, zmq::send_flags::none);
//     zmq::message_t reply;
//     socket_.recv(reply);
//     ZMQMessage reply_message(std::string(reply.data<char>(), reply.data<char>() + reply.size()), get_timestamp());
//     if (reply_message.cmd() == CmdType::ERROR)
//     {
//         throw std::runtime_error("Server returned error: " + reply_message.data_str());
//     }
//     if (reply_message.cmd() != message.cmd())
//     {
//         throw std::runtime_error("Command type mismatch. Sent " + std::to_string(static_cast<int>(message.cmd())) +
//                                  " but received " + std::to_string(static_cast<int>(reply_message.cmd())));
//     }
//     if (reply_message.cmd() == CmdType::GET_LATEST_DATA || reply_message.cmd() == CmdType::REQUEST_WITH_DATA)
//     {
//         last_retrieved_ptrs_ = {reply_message.data_ptr()};
//         return reply_message.data_ptr();
//     }
//     throw std::runtime_error("Invalid command type: " + std::to_string(static_cast<int>(reply_message.cmd())));
// }

std::vector<TimedPtr> ZMQClient::send_request_(ZMQMessage &message)
{
    std::vector<TimedPtr> reply_ptrs;
    std::string serialized = message.serialize();
    zmq::message_t request(serialized.data(), serialized.size());
    socket_.send(request, zmq::send_flags::none);
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
    if (reply_message.cmd() == CmdType::PEEK_DATA || reply_message.cmd() == CmdType::POP_DATA)
    {
        last_retrieved_ptrs_ = reply_message.data_ptrs();
        return reply_message.data_ptrs();
    }
    throw std::runtime_error("Invalid command type: " + std::to_string(static_cast<int>(reply_message.cmd())));
}
