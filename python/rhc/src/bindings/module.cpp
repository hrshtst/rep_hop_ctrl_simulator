#include <pybind11/pybind11.h>

namespace py = pybind11;

// Each binding area registers itself here as it is implemented, e.g.
//   void register_vec(py::module_ &);
// keeping this translation unit a thin assembly point.

PYBIND11_MODULE(_rhc, m) {
  m.doc() = "Python bindings for the rep_hop_ctrl_simulator C library (rhc).";
  m.attr("__version__") = "0.1.0";
}
