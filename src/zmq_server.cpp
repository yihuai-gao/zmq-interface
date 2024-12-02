
#include "zmq_server.h"
#include <filesystem>
#include <spdlog/sinks/stdout_color_sinks.h>

ZMQServer::ZMQServer(const std::string &server_name, const std::string &server_endpoint)
    : server_name_(server_name), context_(1), socket_(context_, zmq::socket_type::rep),
      logger_(spdlog::stdout_color_mt(server_name)), running_(false), steady_clock_start_time_us_(steady_clock_us()),
      poller_timeout_ms_(1000)
{
    logger_->set_pattern("[%H:%M:%S %n %^%l%$] %v");

    // Only accept tcp and ipc endpoints
    if (server_endpoint.find("tcp://") != 0 && server_endpoint.find("ipc://") != 0)
    {
        throw std::invalid_argument("Server endpoint must start with tcp:// or ipc://");
    }
    if (server_endpoint.find("ipc://") == 0)
    {
        // Create the directory if it does not exist
        std::string directory = server_endpoint.substr(6, server_endpoint.find_last_of('/') - 6);
        if (!directory.empty())
        {
            std::filesystem::create_directories(directory);
        }
    }
    socket_.bind(server_endpoint);
    running_ = true;
    poller_item_ = {socket_, 0, ZMQ_POLLIN, 0};
    background_thread_ = std::thread(&ZMQServer::background_loop_, this);
    data_topics_ = std::unordered_map<std::string, DataTopic>();
}

ZMQServer::~ZMQServer()
{
    running_ = false;
    background_thread_.join();
    socket_.close();
    context_.close();
}

void ZMQServer::add_topic(const std::string &topic, double max_remaining_time)
{
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    auto it = data_topics_.find(topic);
    if (it != data_topics_.end())
    {
        logger_->warn("Topic `{}` already exists. Ignoring the request to add it again.", topic);
        return;
    }
    data_topics_.insert({topic, DataTopic(topic, max_remaining_time)});
    logger_->info("Added topic `{}` with max remaining time {}s.", topic, max_remaining_time);
}

void ZMQServer::put_data(const std::string &topic, const PyBytes &data)
{
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn(
            "Received data for unknown topic {}. Please first call add_topic to add it into the recorded topics.",
            topic);
        return;
    }
    PyBytesPtr data_ptr = std::make_shared<PyBytes>(data);

    it->second.add_data_ptr(data_ptr, get_timestamp());
}

pybind11::tuple ZMQServer::peek_data(const std::string &topic, std::string end_type_str, int n)
{
    EndType end_type = str_to_end_type(end_type_str);
    std::vector<TimedPtr> ptrs = peek_data_ptrs_(topic, end_type, n);
    pybind11::list data;
    pybind11::list timestamps;
    for (const TimedPtr ptr : ptrs)
    {
        data.append(*std::get<0>(ptr));
        timestamps.append(std::get<1>(ptr));
    }
    return pybind11::make_tuple(data, timestamps);
}

pybind11::tuple ZMQServer::pop_data(const std::string &topic, std::string end_type_str, int n)
{
    EndType end_type = str_to_end_type(end_type_str);
    std::vector<TimedPtr> ptrs = pop_data_ptrs_(topic, end_type, n);
    pybind11::list data;
    pybind11::list timestamps;
    for (const TimedPtr ptr : ptrs)
    {
        data.append(*std::get<0>(ptr));
        timestamps.append(std::get<1>(ptr));
    }
    return pybind11::make_tuple(data, timestamps);
}

std::unordered_map<std::string, int> ZMQServer::get_topic_status()
{
    std::unordered_map<std::string, int> result;
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    for (auto &pair : data_topics_)
    {
        result[pair.first] = pair.second.size();
    }
    return result;
}

double ZMQServer::get_timestamp()
{
    return static_cast<double>(steady_clock_us() - steady_clock_start_time_us_) / 1e6;
}

void ZMQServer::reset_start_time(int64_t system_time_us)
{
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    logger_->info("Resetting start time. Will clear all data stored before this time");
    for (auto &pair : data_topics_)
    {
        pair.second.clear_data();
    }
    // Use system time to make sure different servers and clients are synchronized
    steady_clock_start_time_us_ = steady_clock_us() + (system_time_us - system_clock_us());
}

std::vector<TimedPtr> ZMQServer::peek_data_ptrs_(const std::string &topic, EndType end_type, int32_t n)
{
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested last k data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return {};
    }
    return it->second.peek_data_ptrs(end_type, n);
}

std::vector<TimedPtr> ZMQServer::pop_data_ptrs_(const std::string &topic, EndType end_type, int32_t n)
{
    std::lock_guard<std::mutex> lock(data_topic_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested last k data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return {};
    }
    return it->second.pop_data_ptrs(end_type, n);
}

void ZMQServer::process_request_(ZMQMessage &message)
{
    switch (message.cmd())
    {
    case CmdType::PEEK_DATA:
    case CmdType::POP_DATA: {
        std::string error_message = "";
        if (message.end_type() == EndType::NONE)
        {
            error_message.append("End type cannot be NONE for PEEK_DATA command. ");
        }
        if (message.data_str().length() != sizeof(int32_t))
        {
            error_message.append("Data length should be the same as an integer, but got ");
            error_message.append(std::to_string(message.data_str().length()));
            error_message.append(" bytes.");
        }
        if (!error_message.empty())
        {
            logger_->error(error_message);
            ZMQMessage reply(message.topic(), CmdType::ERROR, EndType::NONE, get_timestamp(), error_message);
            std::string reply_data = reply.serialize();
            socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
            break;
        }

        int32_t n = bytes_to_int32(message.data_str());
        std::vector<TimedPtr> ptrs = message.cmd() == CmdType::PEEK_DATA
                                         ? peek_data_ptrs_(message.topic(), message.end_type(), n)
                                         : pop_data_ptrs_(message.topic(), message.end_type(), n);
        ZMQMessage reply(message.topic(), message.cmd(), message.end_type(), get_timestamp(), ptrs);
        std::string reply_data = reply.serialize();
        socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        break;
    }

        // case CmdType::REQUEST_WITH_DATA: {
        //     if (!request_with_data_handler_initialized_)
        //     {
        //         std::string error_message = "Request with data handler not initialized";
        //         logger_->error(error_message);
        //         ZMQMessage reply(message.topic(), CmdType::ERROR, error_message, get_timestamp());
        //         std::string reply_data = reply.serialize();
        //         socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        //         break;
        //     }
        //     else
        //     {
        //         ZMQMessage reply(message.topic(), CmdType::REQUEST_WITH_DATA,
        //                          request_with_data_handler_(message.data_ptr()));
        //         std::string reply_data = reply.serialize();
        //         socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        //         break;
        //     }
        // }

        // case CmdType::SYNCHRONIZE_TIME

    default: {
        std::string error_message = "Received unknown command: " + std::to_string(static_cast<int>(message.cmd()));
        logger_->error(error_message);
        ZMQMessage reply(message.topic(), CmdType::ERROR, EndType::NONE, get_timestamp(), error_message);
        std::string reply_data = reply.serialize();
        socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        break;
    }
    }
}

void ZMQServer::background_loop_()
{
    while (running_)
    {

        zmq::poll(&poller_item_, 1, poller_timeout_ms_.count());

        zmq::message_t request;
        if (poller_item_.revents & ZMQ_POLLIN)
        {
            socket_.recv(request);
            ZMQMessage message(std::string(request.data<char>(), request.data<char>() + request.size()));
            process_request_(message);
        }
    }
}
