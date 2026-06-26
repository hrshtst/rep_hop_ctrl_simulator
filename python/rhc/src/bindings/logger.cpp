#include <stdexcept>
#include <string>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_logger(py::module_ &m) {
  py::class_<Logger>(m, "Logger", "A CSV file logger for simulation output.")
      .def(py::init([]() {
        auto *self = new Logger();
        logger_init(&self->l);
        self->inited = true;
        return self;
      }))
      .def(
          "open",
          [](Logger &self, const std::string &filename) {
            if (logger_open(&self.l, filename.c_str()) == nullptr) {
              throw std::runtime_error("cannot open log file: " + filename);
            }
          },
          py::arg("filename"))
      .def("close", [](Logger &self) { logger_close(&self.l); })
      .def_property_readonly("is_open", [](Logger &self) { return static_cast<bool>(logger_is_open(&self.l)); })
      .def_property_readonly("filename", [](Logger &self) { return std::string(logger_filename(&self.l)); })
      .def(
          "set_eol", [](Logger &self, const std::string &eol) { logger_set_eol(&self.l, eol.c_str()); }, py::arg("eol"));
}

}  // namespace rhcpy
