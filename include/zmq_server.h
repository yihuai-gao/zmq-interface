#pragma once

#include <zmq.hpp>

#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

class ZMQServer
{
  public:
    ZMQServer(const std::string &server_endpoint, double max_remaining_time, int max_queue_size,
              std::vector<std::string> topics);
    ~ZMQServer();

    void put_data(const std::string &topic, const std::vector<char> &data);
    std::vector<char> get_latest_data(const std::string &topic);
    std::vector<std::vector<char>> get_all_data(const std::string &topic);
    std::vector<std::vector<char>> get_last_k_data(const std::string &topic, int k);

    void set_request_with_data_handler(std::function<std::vector<char>(const std::vector<char> &)> handler);

  private:
    double max_remaining_time_;
    int max_queue_size_;
    bool running_;
    bool request_with_data_handler_initialized_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::thread background_thread_;

    std::mutex data_mutex_;

    std::unordered_map<std::string, std::deque<std::vector<char>>> topic_queues_;

    std::shared_ptr<spdlog::logger> logger_;

    void process_request_(const ZMQMessage &message);

    std::function<std::vector<char>(const std::vector<char> &)> request_with_data_handler_;

    std::vector<char> serialize_multiple_data_(const std::vector<std::vector<char>> &data);
    void background_loop_();
};
