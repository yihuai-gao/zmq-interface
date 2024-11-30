#pragma once

#include <zmq.hpp>

#include <string>
#include <vector>

#include "common.h"
#include "zmq_message.h"

class ZMQClient
{
  public:
    ZMQClient(const std::string &server_endpoint);
    ~ZMQClient();

    PythonBytes request_latest(const std::string &topic);
    std::vector<PythonBytes> request_all(const std::string &topic);
    std::vector<PythonBytes> request_last_k(const std::string &topic, int k);
    PythonBytes request_with_data(const std::string &topic, const PythonBytes &data);

  private:
    std::vector<PythonBytes> deserialize_multiple_data_(const std::string &data);
    PythonBytes send_message_(const ZMQMessage &message);
    zmq::context_t context_;
    zmq::socket_t socket_;
};
