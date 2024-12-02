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
    ZMQServer(const std::string &server_name, const std::string &server_endpoint);
    ~ZMQServer();
    void add_topic(const std::string &topic, double max_remaining_time);
    void put_data(const std::string &topic, const PyBytes &data);
    pybind11::tuple peek_data(const std::string &topic, std::string end_type_str, int32_t n);
    pybind11::tuple pop_data(const std::string &topic, std::string end_type_str, int32_t n);
    double get_timestamp();
    void reset_start_time(int64_t system_time_us);

    // void set_request_with_data_handler(std::function<PyBytes(const PyBytes)> handler);
    std::unordered_map<std::string, int> get_topic_status();

  private:
    const std::string server_name_;
    bool running_;
    bool request_with_data_handler_initialized_;
    int64_t steady_clock_start_time_us_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    zmq::pollitem_t poller_item_;
    const std::chrono::milliseconds poller_timeout_ms_;
    std::thread background_thread_;
    std::mutex data_topic_mutex_;

    std::unordered_map<std::string, DataTopic> data_topics_;
    std::shared_ptr<spdlog::logger> logger_;

    void process_request_(ZMQMessage &message);

    std::vector<TimedPtr> peek_data_ptrs_(const std::string &topic, EndType end_type, int k);
    std::vector<TimedPtr> pop_data_ptrs_(const std::string &topic, EndType end_type, int k);

    std::function<TimedPtr(const TimedPtr)> request_with_data_handler_;

    void background_loop_();
};
