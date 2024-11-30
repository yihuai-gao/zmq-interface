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

    PythonBytesPtr request_latest(const std::string &topic);
    std::vector<PythonBytesPtr> request_all(const std::string &topic);
    std::vector<PythonBytesPtr> request_last_k(const std::string &topic, int k);
    PythonBytesPtr request_with_data(const std::string &topic, const PythonBytesPtr data_ptr);

  private:
    std::vector<PythonBytesPtr> deserialize_multiple_data_(const std::string &data);
    std::vector<PythonBytesPtr> send_message_(const ZMQMessage &message);
    zmq::context_t context_;
    zmq::socket_t socket_;
};
