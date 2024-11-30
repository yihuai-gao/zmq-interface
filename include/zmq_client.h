#pragma once

#include <zmq.hpp>

#include <string>
#include <vector>

#include "zmq_message.h"

class ZMQClient
{
  public:
    ZMQClient(const std::string &server_endpoint);
    ~ZMQClient();

    std::vector<char> request_latest(const std::string &topic);
    std::vector<std::vector<char>> request_last_k(const std::string &topic, int k);
    std::vector<char> request_with_data(const std::string &topic, const std::vector<char> &data);

  private:
    std::vector<std::vector<char>> deserialize_multiple_data_(const std::vector<char> &data);
    std::vector<char> send_message_(const ZMQMessage &message);
    zmq::context_t context_;
    zmq::socket_t socket_;
};
