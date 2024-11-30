#pragma once
#include "common.h"
#include <deque>
#include <string>
#include <vector>
class DataTopic
{
  public:
    DataTopic(const std::string &topic_name, double max_remaining_time);

    void add_data(const PythonBytes &data, double timestamp);

    PythonBytes get_latest_data();

    std::vector<PythonBytes> get_all_data();

    std::vector<PythonBytes> get_last_k_data(int k);

  private:
    std::string topic_name_;
    double max_remaining_time_;
    std::deque<PythonBytes> data_;
    std::deque<double> timestamps_;
};