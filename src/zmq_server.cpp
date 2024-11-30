
#include "zmq_server.h"
#include <spdlog/sinks/stdout_color_sinks.h>

ZMQServer::ZMQServer(const std::string &endpoint)
    : context_(1), socket_(context_, zmq::socket_type::rep), logger_(spdlog::stdout_color_mt("ZMQServer")),
      running_(false), start_time_(get_time_us())
{
    socket_.bind(endpoint);
    running_ = true;
    background_thread_ = std::thread(&ZMQServer::background_loop_, this);
    data_topics_ = std::unordered_map<std::string, DataTopic>();
}

ZMQServer::~ZMQServer()
{
    socket_.close();
    context_.close();
    running_ = false;
    background_thread_.join();
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
    data_topics_.insert({"topic", DataTopic(topic, max_remaining_time)});
    logger_->info("Added topic {} with max remaining time {}.", topic, max_remaining_time);
}

void ZMQServer::put_data(const std::string &topic, const std::vector<char> &data)
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
    it->second.add_data(data, get_time_us() - start_time_);
}

std::vector<char> ZMQServer::get_latest_data(const std::string &topic)
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
    return it->second.get_latest_data();
}

std::vector<std::vector<char>> ZMQServer::get_all_data(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = data_topics_.find(topic);
    if (it == data_topics_.end())
    {
        logger_->warn("Requested all data for unknown topic {}. Please first call add_topic to add it into the "
                      "recorded topics.",
                      topic);
        return std::vector<std::vector<char>>();
    }
    return it->second.get_all_data();
}

std::vector<std::vector<char>> ZMQServer::get_last_k_data(const std::string &topic, int k)
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

void ZMQServer::set_request_with_data_handler(std::function<std::vector<char>(const std::vector<char> &)> handler)
{
    request_with_data_handler_initialized_ = true;
    request_with_data_handler_ = handler;
}

std::vector<char> ZMQServer::serialize_multiple_data_(const std::vector<std::vector<char>> &data)
{
    std::vector<char> result;
    // first 2 bytes are the number of data
    std::vector<char> num_data(2);
    *reinterpret_cast<short *>(num_data.data()) = static_cast<short>(data.size());
    result.insert(result.end(), num_data.begin(), num_data.end());
    // next num_data * sizeof(int) bytes are the lengths of the packages
    std::vector<char> package_lengths(data.size() * sizeof(int));
    for (int i = 0; i < data.size(); ++i)
    {
        *reinterpret_cast<int *>(package_lengths.data() + i * sizeof(int)) = static_cast<int>(data[i].size());
    }
    result.insert(result.end(), package_lengths.begin(), package_lengths.end());
    // the rest of the bytes are the data
    for (const std::vector<char> &d : data)
    {
        result.insert(result.end(), d.begin(), d.end());
    }
    return result;
}

void ZMQServer::process_request_(const ZMQMessage &message)
{
    ZMQMessage reply(message.topic(), message.cmd(), {});
    switch (message.cmd())
    {
    case CmdType::GET_LATEST_DATA:
        reply = ZMQMessage(message.topic(), CmdType::GET_LATEST_DATA, get_latest_data(message.topic()));
        break;
    case CmdType::GET_ALL_DATA:
        reply =
            ZMQMessage(message.topic(), CmdType::GET_ALL_DATA, serialize_multiple_data_(get_all_data(message.topic())));
        break;
    case CmdType::GET_LAST_K_DATA:
        if (message.data().size() != sizeof(int))
        {
            std::string error_message = "Invalid data size when requesting last k data: got " +
                                        std::to_string(message.data().size()) + " bytes, expected " +
                                        std::to_string(sizeof(int)) + " bytes";
            logger_->error(error_message);
            reply = ZMQMessage(message.topic(), CmdType::ERROR,
                               std::vector<char>(error_message.begin(), error_message.end()));
            break;
        }
        reply = ZMQMessage(message.topic(), CmdType::GET_LAST_K_DATA,
                           serialize_multiple_data_(get_last_k_data(message.topic(), message.data().size())));
        break;
    case CmdType::REQUEST_WITH_DATA:
        if (!request_with_data_handler_initialized_)
        {
            std::string error_message = "Request with data handler not initialized";
            logger_->error(error_message);
            reply = ZMQMessage(message.topic(), CmdType::ERROR,
                               std::vector<char>(error_message.begin(), error_message.end()));
            break;
        }
        reply = ZMQMessage(message.topic(), CmdType::REQUEST_WITH_DATA, request_with_data_handler_(message.data()));
        break;
    default:
        std::string error_message = "Received unknown command: " + std::to_string(static_cast<int>(message.cmd()));
        logger_->error(error_message);
        reply =
            ZMQMessage(message.topic(), CmdType::ERROR, std::vector<char>(error_message.begin(), error_message.end()));
        break;
    }
    std::vector<char> reply_data = reply.serialize();
    socket_.send(zmq::message_t(reply_data.data(), reply_data.size()), zmq::send_flags::none);
}

void ZMQServer::background_loop_()
{
    while (running_)
    {
        zmq::message_t request;
        socket_.recv(request);
        ZMQMessage message(std::vector<char>(request.data<char>(), request.data<char>() + request.size()));
        process_request_(message);
    }
}
