#pragma once

#include <zmq.hpp>

#include <deque>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

  private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::thread background_thread_;

    std::unordered_map<std::string, std::deque<std::vector<char>>> topic_queues_;

    void process_request_(const std::string &topic, const std::vector<char> &message);
    void background_loop_();
};
