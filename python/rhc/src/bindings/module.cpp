#include <pybind11/pybind11.h>

#include "register.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_rhc, m) {
  m.doc() = "Python bindings for the rep_hop_ctrl_simulator C library (rhc).";
  m.attr("__version__") = "0.1.0";

  rhcpy::register_vec(m);
  rhcpy::register_complex(m);
  rhcpy::register_cmd(m);
  rhcpy::register_model(m);
  rhcpy::register_ctrl(m);
  rhcpy::register_dynmorph(m);
  rhcpy::register_simulator(m);
}
