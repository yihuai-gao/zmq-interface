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

std::vector<TimedPtr> DataTopic::peek_data_ptrs(EndType end_type, int k)
{
    if (k < -1)
    {
        k = -1;
    }
    if (end_type == EndType::LATEST)
    {
        if (data_.size() < k || k == -1)
            return std::vector<TimedPtr>(data_.begin(), data_.end());
        return std::vector<TimedPtr>(data_.end() - k, data_.end());
    }
    else if (end_type == EndType::EARLIEST)
    {
        if (data_.size() < k || k == -1)
            return std::vector<TimedPtr>(data_.begin(), data_.end());
        return std::vector<TimedPtr>(data_.begin(), data_.begin() + k);
    }
}

std::vector<TimedPtr> DataTopic::pop_data_ptrs(EndType end_type, int k)
{
    auto ret = peek_data_ptrs(end_type, k);
    if (end_type == EndType::LATEST)
    {
        data_.erase(data_.end() - k, data_.end());
    }
    else if (end_type == EndType::EARLIEST)
    {
        data_.erase(data_.begin(), data_.begin() + k);
    }
    return ret;
}

void DataTopic::clear_data()
{
    data_.clear();
}

int DataTopic::size() const
{
    return data_.size();
}
