#pragma once
#include <deque>
#include <string>
#include <vector>

class DataTopic
{
  public:
    DataTopic(const std::string &topic_name, double max_remaining_time);

    void add_data(const std::vector<char> &data, double timestamp);

    std::vector<char> get_latest_data();

    std::vector<std::vector<char>> get_all_data();

    std::vector<std::vector<char>> get_last_k_data(int k);

  private:
    std::string topic_name_;
    double max_remaining_time_;
    std::deque<std::vector<char>> data_;
    std::deque<double> timestamps_;
};