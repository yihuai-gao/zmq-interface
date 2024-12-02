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

std::vector<TimedPtr> DataTopic::peek_data_ptrs(EndType end_type, int32_t n)
{
    if (data_.empty())
    {
        return std::vector<TimedPtr>();
    }
    if (n < 0 || n > data_.size())
    {
        n = data_.size();
    }
    if (end_type == EndType::LATEST)
    {
        return std::vector<TimedPtr>(data_.end() - n, data_.end());
    }
    else if (end_type == EndType::EARLIEST)
    {
        return std::vector<TimedPtr>(data_.begin(), data_.begin() + n);
    }
    else
    {
        throw std::runtime_error("Invalid end type");
    }
}

std::vector<TimedPtr> DataTopic::pop_data_ptrs(EndType end_type, int32_t n)
{
    if (data_.empty())
    {
        return std::vector<TimedPtr>();
    }
    if (n < 0 || n > data_.size())
    {
        n = data_.size();
    }
    std::vector<TimedPtr> ret = peek_data_ptrs(end_type, n);

    if (end_type == EndType::LATEST)
    {
        for (int i = 0; i < n; i++)
        {
            data_.pop_back();
        }
    }
    else if (end_type == EndType::EARLIEST)
    {
        for (int i = 0; i < n; i++)
        {
            data_.pop_front();
        }
    }
    else
    {
        throw std::runtime_error("Invalid end type");
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
