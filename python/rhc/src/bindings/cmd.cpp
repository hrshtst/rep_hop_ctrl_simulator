#include <cstdio>
#include <string>

#include "register.hpp"
#include "rhc_api.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_cmd(py::module_ &m) {
  py::class_<cmd_t>(m, "Cmd", "Controller command parameters: the COM heights za/zh/zm/zb.")
      .def(py::init([]() {
             cmd_t c;
             cmd_default_init(&c);
             return c;
           }),
           "Create a command initialised with the default hopping heights.")
      .def_readwrite("za", &cmd_t::za, "Target apex height.")
      .def_readwrite("zh", &cmd_t::zh, "Lift-off / touchdown height.")
      .def_readwrite("zm", &cmd_t::zm, "Standing (equilibrium) height.")
      .def_readwrite("zb", &cmd_t::zb, "Crouching bottom height.")
      .def(
          "set",
          [](cmd_t &self, double za, double zh, double zm, double zb) { cmd_set(&self, za, zh, zm, zb); },
          py::arg("za"), py::arg("zh"), py::arg("zm"), py::arg("zb"))
      .def(
          "copy",
          [](cmd_t &self) {
            cmd_t d;
            cmd_copy(&self, &d);
            return d;
          },
          "Return an independent copy of this command.")
      .def("__repr__", [](cmd_t &c) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Cmd(za=%g, zh=%g, zm=%g, zb=%g)", c.za, c.zh, c.zm, c.zb);
        return std::string(buf);
      });
}

}  // namespace rhcpy
