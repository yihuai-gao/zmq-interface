#pragma once

#include <zmq.hpp>

#include "common.h"
#include "zmq_message.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class ZMQClient
{
  public:
    ZMQClient(const std::string &client_name, const std::string &server_endpoint);
    ~ZMQClient();

    pybind11::list request_latest(const std::string &topic);
    pybind11::list request_all(const std::string &topic);
    pybind11::list request_last_k(const std::string &topic, uint32_t k);
    pybind11::list request_with_data(const std::string &topic, const PyBytes data_ptr);
    pybind11::list get_last_retrieved_data();

    double get_timestamp();
    void reset_start_time(int64_t system_time_us);

  private:
    std::vector<TimedPtr> deserialize_multiple_data_(const std::string &data);
    TimedPtr send_single_block_request_(const ZMQMessage &message);
    std::vector<TimedPtr> send_multi_block_request_(const ZMQMessage &message);

    std::string client_name_;
    std::shared_ptr<spdlog::logger> logger_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::vector<TimedPtr> last_retrieved_ptrs_;
    int64_t steady_clock_start_time_us_;
};
