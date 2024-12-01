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

    PyBytes request_latest(const std::string &topic);
    std::vector<PyBytes> request_all(const std::string &topic);
    std::vector<PyBytes> request_last_k(const std::string &topic, int k);
    PyBytes request_with_data(const std::string &topic, const PyBytes data_ptr);
    std::vector<PyBytes> get_last_retrieved_data();

  private:
    std::vector<PyBytesPtr> deserialize_multiple_data_(const std::string &data);
    PyBytesPtr send_single_block_request_(const ZMQMessage &message);
    std::vector<PyBytesPtr> send_multi_block_request_(const ZMQMessage &message);

    zmq::context_t context_;
    zmq::socket_t socket_;
    std::vector<PyBytesPtr> last_retrieved_ptrs_;
};
