#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

#include "TCPClient.h"
#include "TCPMsg.h"

namespace py = pybind11;
using namespace TCPConn;

// Trampoline for ITCPClient<TCPMsg>
struct PyITCPClientTCPMsg : ITCPClient<TCPMsg> {
    using ITCPClient<TCPMsg>::ITCPClient;
    void OnConnected() override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE(void, ITCPClient<TCPMsg>, OnConnected);
    }
    void OnDisconnected() override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE(void, ITCPClient<TCPMsg>, OnDisconnected);
    }
    void OnMessage(TCPMsg& msg) override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE_PURE(void, ITCPClient<TCPMsg>, OnMessage, msg);
    }
};

// Trampoline for ITCPClient<TCPRawMsg>
struct PyITCPClientTCPRawMsg : ITCPClient<TCPRawMsg> {
    using ITCPClient<TCPRawMsg>::ITCPClient;
    void OnConnected() override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE(void, ITCPClient<TCPRawMsg>, OnConnected);
    }
    void OnDisconnected() override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE(void, ITCPClient<TCPRawMsg>, OnDisconnected);
    }
    void OnMessage(TCPRawMsg& msg) override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE_PURE(void, ITCPClient<TCPRawMsg>, OnMessage, msg);
    }
};

PYBIND11_MODULE(tcpconn_py, m) {
    m.doc() = "Python bindings for TCPConn TCPClient";

    py::class_<TCPMsgHeader>(m, "TCPMsgHeader")
        .def(py::init<>())
        .def_readwrite("type", &TCPMsgHeader::type)
        .def_readwrite("size", &TCPMsgHeader::size);

    py::class_<TCPMsg>(m, "TCPMsg")
        .def(py::init<>())
        .def_readwrite("header", &TCPMsg::header)
        .def_readwrite("body", &TCPMsg::body)
        .def("formatted", &TCPMsg::formatted)
        .def("full_size", &TCPMsg::full_size)
        .def("__repr__", [](const TCPMsg& self){ return self.formatted(); });

    py::class_<TCPRawMsg>(m, "TCPRawMsg")
        .def(py::init<>())
        .def_readwrite("body", &TCPRawMsg::body)
        .def("formatted", &TCPRawMsg::formatted)
        .def("full_size", &TCPRawMsg::full_size)
        .def("to_bytes", [](const TCPRawMsg& self) {
            return py::bytes(reinterpret_cast<const char*>(self.body.data()), self.body.size());
        })
        .def("from_bytes", [](TCPRawMsg& self, py::bytes b) {
            char* buf; py::ssize_t len;
            PYBIND11_BYTES_AS_STRING_AND_SIZE(b.ptr(), &buf, &len);
            self.body.assign(reinterpret_cast<uint8_t*>(buf), reinterpret_cast<uint8_t*>(buf) + len);
        })
        .def("__repr__", [](const TCPRawMsg& self){ return self.formatted(); });

    py::class_<ITCPClient<TCPMsg>, PyITCPClientTCPMsg>(m, "TCPClientMsg")
        .def(py::init<>())
        .def("connect", &ITCPClient<TCPMsg>::Connect, py::arg("host"), py::arg("port"))
        .def("disconnect", &ITCPClient<TCPMsg>::Disconnect)
        .def("is_connected", &ITCPClient<TCPMsg>::IsConnected)
        .def("send", &ITCPClient<TCPMsg>::Send, py::arg("msg"))
        .def("update", &ITCPClient<TCPMsg>::Update, py::arg("wait"), py::arg("max_messages") = static_cast<size_t>(-1), py::call_guard<py::gil_scoped_release>())
        .def("run", &ITCPClient<TCPMsg>::Run, py::call_guard<py::gil_scoped_release>())
        ;

    py::class_<ITCPClient<TCPRawMsg>, PyITCPClientTCPRawMsg>(m, "TCPClientRaw")
        .def(py::init<>())
        .def("connect", &ITCPClient<TCPRawMsg>::Connect, py::arg("host"), py::arg("port"))
        .def("disconnect", &ITCPClient<TCPRawMsg>::Disconnect)
        .def("is_connected", &ITCPClient<TCPRawMsg>::IsConnected)
        .def("send", &ITCPClient<TCPRawMsg>::Send, py::arg("msg"))
        .def("update", &ITCPClient<TCPRawMsg>::Update, py::arg("wait"), py::arg("max_messages") = static_cast<size_t>(-1), py::call_guard<py::gil_scoped_release>())
        .def("run", &ITCPClient<TCPRawMsg>::Run, py::call_guard<py::gil_scoped_release>())
        ;
}
