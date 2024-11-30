#include "zmq_server.h"
#include "zmq_message.h"

ZMQServer::ZMQServer(const std::string &endpoint, double max_remaining_time, int max_queue_size,
                     std::vector<std::string> topics)
    : context_(1), socket_(context_, zmq::socket_type::rep), max_remaining_time_(max_remaining_time),
      max_queue_size_(max_queue_size)
{
    socket_.bind(endpoint);
    for (const std::string &topic : topics)
    {
        topic_queues_[topic] = std::deque<std::vector<char>>();
    }
    running_ = true;
    background_thread_ = std::thread(&ZMQServer::background_loop_, this);
}

ZMQServer::~ZMQServer()
{
    socket_.close();
    context_.close();
    running_ = false;
    background_thread_.join();
}

void ZMQServer::put_data(const std::string &topic, const std::vector<char> &data)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    topic_queues_[topic].push_back(data);
}

std::vector<char> ZMQServer::get_latest_data(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    return topic_queues_[topic].back();
}

std::vector<std::vector<char>> ZMQServer::get_all_data(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    return std::vector<std::vector<char>>(topic_queues_[topic].begin(), topic_queues_[topic].end());
}

std::vector<std::vector<char>> ZMQServer::get_last_k_data(const std::string &topic, int k)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (k > topic_queues_[topic].size())
    {
        k = topic_queues_[topic].size();
        logger_->warn("Requested more data than available for topic {}. Returning {} data instead.", topic, k);
    }
    return std::vector<std::vector<char>>(topic_queues_[topic].end() - k, topic_queues_[topic].end());
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
