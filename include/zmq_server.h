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
    pybind11::list get_latest_data(const std::string &topic);
    pybind11::list get_all_data(const std::string &topic);
    pybind11::list get_last_k_data(const std::string &topic, int k);
    double get_timestamp();
    void reset_start_time(int64_t system_time_us);

    // void set_request_with_data_handler(std::function<PyBytes(const PyBytes)> handler);
    std::vector<std::string> get_all_topic_names();

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
    std::mutex data_mutex_;

    std::unordered_map<std::string, DataTopic> data_topics_;
    std::shared_ptr<spdlog::logger> logger_;

    void process_request_(const ZMQMessage &message);

    TimedPtr get_latest_data_ptr_(const std::string &topic);
    std::vector<TimedPtr> get_all_data_ptrs_(const std::string &topic);
    std::vector<TimedPtr> get_last_k_data_ptrs_(const std::string &topic, int k);

    std::function<TimedPtr(const TimedPtr)> request_with_data_handler_;

    void background_loop_();
};
