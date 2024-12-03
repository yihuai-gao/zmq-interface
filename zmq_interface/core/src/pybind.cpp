#include "common.h"
#include "data_topic.h"
#include "zmq_client.h"
#include "zmq_message.h"
#include "zmq_server.h"
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PyBytesPtr bytes_to_shared_string(py::bytes py_bytes)
{
    return std::make_shared<py::bytes>(py_bytes);
}

PYBIND11_MODULE(zmq_interface, m)
{

    m.def("steady_clock_us", &steady_clock_us);
    m.def("system_clock_us", &system_clock_us);

    py::class_<ZMQClient>(m, "ZMQClient")
        .def(py::init<const std::string &, const std::string &>())
        .def("peek_data", &ZMQClient::peek_data)
        .def("pop_data", &ZMQClient::pop_data)
        .def("get_last_retrieved_data", &ZMQClient::get_last_retrieved_data)
        .def("reset_start_time", &ZMQClient::reset_start_time)
        .def("get_timestamp", &ZMQClient::get_timestamp);

    py::class_<ZMQServer>(m, "ZMQServer")
        .def(py::init<const std::string &, const std::string &>())
        .def("add_topic", &ZMQServer::add_topic)
        .def("put_data", &ZMQServer::put_data)
        .def("peek_data", &ZMQServer::peek_data)
        .def("pop_data", &ZMQServer::pop_data)
        .def("get_topic_status", &ZMQServer::get_topic_status)
        .def("reset_start_time", &ZMQServer::reset_start_time)
        .def("get_timestamp", &ZMQServer::get_timestamp);
}
