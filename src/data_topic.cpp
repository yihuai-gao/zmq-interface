#include "data_topic.h"

DataTopic::DataTopic(const std::string &topic_name, double max_remaining_time)
    : max_remaining_time_(max_remaining_time), topic_name_(topic_name)
{
    data_.clear();
}

void DataTopic::add_data_ptr(const PyBytesPtr data_ptr, double timestamp)
{
    data_.push_back({data_ptr, timestamp});
    while (!data_.empty() && timestamp - std::get<1>(data_.front()) > max_remaining_time_)
    {
        data_.pop_front();
    }
}

std::vector<TimedPtr> DataTopic::get_all_data()
{
    return std::vector<TimedPtr>(data_.begin(), data_.end());
}

std::vector<TimedPtr> DataTopic::get_last_k_data(int k)
{
    if (data_.size() < k)
    {
        k = data_.size();
    }
    return std::vector<TimedPtr>(data_.end() - k, data_.end());
}

TimedPtr DataTopic::get_latest_data_ptr()
{
    if (data_.empty())
    {
        return {};
    }
    return data_.back();
}
