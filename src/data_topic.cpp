#include "data_topic.h"

DataTopic::DataTopic(const std::string &topic_name, double max_remaining_time)
    : max_remaining_time_(max_remaining_time), topic_name_(topic_name)
{
    data_.clear();
    timestamps_.clear();
}

void DataTopic::add_data_ptr(const PyBytesPtr data_ptr, double timestamp)
{
    data_.push_back(data_ptr);
    timestamps_.push_back(timestamp);
}

std::vector<PyBytesPtr> DataTopic::get_all_data()
{
    return std::vector<PyBytesPtr>(data_.begin(), data_.end());
}

std::vector<PyBytesPtr> DataTopic::get_last_k_data(int k)
{
    if (data_.size() < k)
    {
        k = data_.size();
    }
    return std::vector<PyBytesPtr>(data_.end() - k, data_.end());
}

PyBytesPtr DataTopic::get_latest_data_ptr()
{
    if (data_.empty())
    {
        return {};
    }
    return data_.back();
}
