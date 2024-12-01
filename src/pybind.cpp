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
    // py::enum_<CmdType>(m, "CmdType")
    //     .value("GET_LATEST_DATA", CmdType::GET_LATEST_DATA)
    //     .value("GET_ALL_DATA", CmdType::GET_ALL_DATA)
    //     .value("GET_LAST_K_DATA", CmdType::GET_LAST_K_DATA)
    //     .value("REQUEST_WITH_DATA", CmdType::REQUEST_WITH_DATA)
    //     .value("ERROR", CmdType::ERROR)
    //     .value("UNKNOWN", CmdType::UNKNOWN);

    // py::class_<DataTopic>(m, "DataTopic")
    //     .def(py::init<const std::string &, double>())
    //     .def("add_data_ptr", &DataTopic::add_data_ptr)
    //     .def("get_latest_data_ptr", &DataTopic::get_latest_data_ptr)
    //     .def("get_all_data", &DataTopic::get_all_data)
    //     .def("get_last_k_data", &DataTopic::get_last_k_data);

    // py::class_<ZMQMessage>(m, "ZMQMessage")
    //     .def(py::init<const std::string &, CmdType, const PyBytesPtr>())
    //     .def(py::init<const std::string &>())
    //     .def("topic", &ZMQMessage::topic)
    //     .def("cmd", &ZMQMessage::cmd)
    //     .def("data", &ZMQMessage::data_ptr)
    //     .def("serialize", &ZMQMessage::serialize);

    py::class_<ZMQClient>(m, "ZMQClient")
        .def(py::init<const std::string &, const std::string &>())
        .def("request_latest", &ZMQClient::request_latest)
        .def("request_all", &ZMQClient::request_all)
        .def("request_last_k", &ZMQClient::request_last_k)
        .def("get_last_retrieved_data", &ZMQClient::get_last_retrieved_data);
    // .def("request_with_data", &ZMQClient::request_with_data);

    py::class_<ZMQServer>(m, "ZMQServer")
        .def(py::init<const std::string &, const std::string &>())
        .def("add_topic", &ZMQServer::add_topic)
        .def("put_data", &ZMQServer::put_data)
        .def("get_latest_data", &ZMQServer::get_latest_data)
        .def("get_all_data", &ZMQServer::get_all_data)
        .def("get_last_k_data", &ZMQServer::get_last_k_data)
        .def("get_all_topic_names", &ZMQServer::get_all_topic_names);
    // .def("set_request_with_data_handler", &ZMQServer::set_request_with_data_handler);
}
