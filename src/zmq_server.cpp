
#include "zmq_server.h"
#include <spdlog/sinks/stdout_color_sinks.h>

ZMQServer::ZMQServer(const std::string &server_name, const std::string &server_endpoint)
    : server_name_(server_name), context_(1), socket_(context_, zmq::socket_type::rep),
      logger_(spdlog::stdout_color_mt(server_name)), running_(false), steady_clock_start_time_us_(steady_clock_us()),
      poller_timeout_ms_(1000)
{
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
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_topics_.find(topic);
    if (it != data_topics_.end())
    {
        logger_->warn("Topic {} already exists. Ignoring the request to add it again.", topic);
        return;
    }
    data_topics_.insert({topic, DataTopic(topic, max_remaining_time)});
    logger_->info("Added topic {} with max remaining time {}.", topic, max_remaining_time);
}

void ZMQServer::put_data(const std::string &topic, const PyBytes &data)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
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

pybind11::list ZMQServer::get_latest_data(const std::string &topic)
{
    pybind11::list result;
    TimedPtr ptr = get_latest_data_ptr_(topic);
    result.append(*std::get<0>(ptr));
    result.append(std::get<1>(ptr));
    return result;
}

pybind11::list ZMQServer::get_all_data(const std::string &topic)
{
    std::vector<TimedPtr> ptrs = get_all_data_ptrs_(topic);
    pybind11::list result;
    for (const TimedPtr ptr : ptrs)
    {
        pybind11::list single_result;
        single_result.append(*std::get<0>(ptr));
        single_result.append(std::get<1>(ptr));
        result.append(single_result);
    }
    return result;
}

pybind11::list ZMQServer::get_last_k_data(const std::string &topic, int k)
{
    std::vector<TimedPtr> ptrs = get_last_k_data_ptrs_(topic, k);
    pybind11::list result;
    for (const TimedPtr ptr : ptrs)
    {
        pybind11::list single_result;
        single_result.append(*std::get<0>(ptr));
        single_result.append(std::get<1>(ptr));
        result.append(single_result);
    }
    return result;
}

std::vector<std::string> ZMQServer::get_all_topic_names()
{
    std::vector<std::string> result;
    for (const auto &pair : data_topics_)
    {
        result.push_back(pair.first);
    }
    return result;
}

double ZMQServer::get_timestamp()
{
    return static_cast<double>(steady_clock_us() - steady_clock_start_time_us_) / 1e6;
}

void ZMQServer::reset_start_time(int64_t system_time_us)
{
    // Use system time to make sure different servers and clients are synchronized
    steady_clock_start_time_us_ = steady_clock_us() + (system_time_us - system_clock_us());
}

TimedPtr ZMQServer::get_latest_data_ptr_(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested latest data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return {};
    }
    return it->second.get_latest_data_ptr();
}

std::vector<TimedPtr> ZMQServer::get_all_data_ptrs_(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested all data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return std::vector<TimedPtr>();
    }
    return it->second.get_all_data();
}

std::vector<TimedPtr> ZMQServer::get_last_k_data_ptrs_(const std::string &topic, int k)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested last k data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return {};
    }
    return it->second.get_last_k_data(k);
}

// void ZMQServer::set_request_with_data_handler(std::function<PyBytesPtr(const PyBytesPtr)> handler)
// {
//     request_with_data_handler_initialized_ = true;
//     request_with_data_handler_ = handler;
// }

void ZMQServer::process_request_(const ZMQMessage &message)
{
    switch (message.cmd())
    {
    case CmdType::GET_LATEST_DATA: {
        ZMQMessage reply(message.topic(), CmdType::GET_LATEST_DATA, get_latest_data_ptr_(message.topic()));
        std::string reply_data = reply.serialize();
        socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        break;
    }
    case CmdType::GET_ALL_DATA: {
        ZMQMultiPtrMessage reply(message.topic(), CmdType::GET_ALL_DATA, get_all_data_ptrs_(message.topic()));
        std::string reply_data = reply.serialize();
        socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
        break;
    }
    case CmdType::GET_LAST_K_DATA: {
        if (message.data_str().size() != sizeof(uint32_t))
        {
            std::string error_message = "Invalid data size when requesting last k data: got " +
                                        std::to_string(message.data_str().size()) + " bytes, expected " +
                                        std::to_string(sizeof(uint32_t)) + " bytes";
            logger_->error(error_message);
            ZMQMultiPtrMessage reply(message.topic(), CmdType::ERROR, error_message);
            std::string reply_data = reply.serialize();
            socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
            break;
        }
        else
        {
            uint32_t k = bytes_to_uint32(message.data_str());
            std::vector<TimedPtr> pointers = get_last_k_data_ptrs_(message.topic(), k);
            ZMQMultiPtrMessage reply(message.topic(), CmdType::GET_LAST_K_DATA, pointers);
            std::string reply_data = reply.serialize();
            socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
            break;
        }
        break;
    }

    case CmdType::REQUEST_WITH_DATA: {
        if (!request_with_data_handler_initialized_)
        {
            std::string error_message = "Request with data handler not initialized";
            logger_->error(error_message);
            ZMQMessage reply(message.topic(), CmdType::ERROR, error_message, get_timestamp());
            std::string reply_data = reply.serialize();
            socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
            break;
        }
        else
        {
            ZMQMessage reply(message.topic(), CmdType::REQUEST_WITH_DATA,
                             request_with_data_handler_(message.data_ptr()));
            std::string reply_data = reply.serialize();
            socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
            break;
        }
    }

    default: {
        std::string error_message = "Received unknown command: " + std::to_string(static_cast<int>(message.cmd()));
        logger_->error(error_message);
        ZMQMessage reply(message.topic(), CmdType::ERROR, error_message, get_timestamp());
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
