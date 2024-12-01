#pragma once
#include "common.h"
#include <deque>
#include <string>
#include <vector>
class DataTopic
{
  public:
    DataTopic(const std::string &topic_name, double max_remaining_time);

    void add_data_ptr(const PyBytesPtr data_ptr, double timestamp);

    PyBytesPtr get_latest_data_ptr();

    std::vector<PyBytesPtr> get_all_data();

    std::vector<PyBytesPtr> get_last_k_data(int k);

  private:
    std::string topic_name_;
    double max_remaining_time_;
    std::deque<PyBytesPtr> data_;
    std::deque<double> timestamps_;
};