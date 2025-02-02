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

    pybind11::tuple peek_data(const std::string &topic, std::string end_type, int32_t n);
    pybind11::tuple pop_data(const std::string &topic, std::string end_type, int32_t n);
    pybind11::tuple get_last_retrieved_data();

    double get_timestamp();
    void reset_start_time(int64_t system_time_us);

  private:
    std::vector<TimedPtr> deserialize_multiple_data_(const std::string &data);
    // TimedPtr send_single_block_request_(const ZMQMessage &message);
    std::vector<TimedPtr> send_request_(ZMQMessage &message);

    std::string client_name_;
    std::shared_ptr<spdlog::logger> logger_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::vector<TimedPtr> last_retrieved_ptrs_;
    int64_t steady_clock_start_time_us_;
};
