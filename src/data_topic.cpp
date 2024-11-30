#include "data_topic.h"

DataTopic::DataTopic(const std::string &topic_name, double max_remaining_time)
    : max_remaining_time_(max_remaining_time), topic_name_(topic_name)
{
    data_.clear();
    timestamps_.clear();
}

void DataTopic::add_data_ptr(const PythonBytesPtr data_ptr, double timestamp)
{
    data_.push_back(data_ptr);
    timestamps_.push_back(timestamp);
}

std::vector<PythonBytesPtr> DataTopic::get_all_data()
{
    return std::vector<PythonBytesPtr>(data_.begin(), data_.end());
}

std::vector<PythonBytesPtr> DataTopic::get_last_k_data(int k)
{
    if (data_.size() < k)
    {
        k = data_.size();
    }
    return std::vector<PythonBytesPtr>(data_.end() - k, data_.end());
}

PythonBytesPtr DataTopic::get_latest_data_ptr()
{
    if (data_.empty())
    {
        return {};
    }
    return data_.back();
}
