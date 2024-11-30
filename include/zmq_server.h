#pragma once

#include <zmq.hpp>

#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "data_topic.h"
#include "spdlog/spdlog.h"
#include "zmq_message.h"
class ZMQServer
{
  public:
    ZMQServer(const std::string &server_endpoint);
    ~ZMQServer();
    void add_topic(const std::string &topic, double max_remaining_time);
    void put_data(const std::string &topic, const PythonBytesPtr data_ptr);
    PythonBytesPtr get_latest_data_ptr(const std::string &topic);
    std::vector<PythonBytesPtr> get_all_data(const std::string &topic);
    std::vector<PythonBytesPtr> get_last_k_data(const std::string &topic, int k);

    void set_request_with_data_handler(std::function<PythonBytesPtr(const PythonBytesPtr)> handler);
    std::vector<std::string> get_all_topic_names();

  private:
    bool running_;
    bool request_with_data_handler_initialized_;
    double start_time_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::thread background_thread_;
    std::mutex data_mutex_;

    std::unordered_map<std::string, DataTopic> data_topics_;
    std::shared_ptr<spdlog::logger> logger_;

    void process_request_(const ZMQMessage &message);

    std::function<PythonBytesPtr(const PythonBytesPtr)> request_with_data_handler_;

    // PythonBytesPtr serialize_multiple_data_(const std::vector<PythonBytesPtr> &data);
    void background_loop_();
};
