#include <pybind11/numpy.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

// C-linkage trampoline: the ODE integrator calls this each evaluation; it
// forwards to the Python right-hand side stored on the Ode wrapper (passed as
// the util pointer). The GIL is held because ode_update runs synchronously
// inside the bound Python call.
extern "C" vec_t rhcpy_ode_py_thunk(double t, vec_t x, void *util, vec_t v) {
  auto *self = static_cast<rhcpy::Ode *>(util);
  py::array_t<double> xa(static_cast<py::ssize_t>(vec_size(x)), vec_buf(x));
  py::object result = self->f(t, xa);
  auto arr = result.cast<py::array_t<double>>();
  const py::buffer_info info = arr.request();
  if (static_cast<std::size_t>(info.size) != vec_size(v)) {
    throw std::runtime_error("ODE rhs returned the wrong number of elements");
  }
  const auto *rp = static_cast<const double *>(info.ptr);
  for (std::size_t i = 0; i < vec_size(v); ++i) {
    vec_set_elem(v, i, rp[i]);
  }
  return v;
}

namespace rhcpy {

void register_ode(py::module_ &m) {
  py::class_<Ode>(m, "Ode", "An ODE integrator (Euler or RK4) driven by a Python right-hand side.")
      .def(py::init([](const std::string &method, int dim, const py::function &rhs) {
             auto *self = new Ode();
             self->f = rhs;
             if (method == "rk4") {
               ode_assign(&self->o, rk4);
             } else if (method == "euler") {
               ode_assign(&self->o, euler);
             } else {
               delete self;
               throw std::invalid_argument("method must be 'rk4' or 'euler'");
             }
             if (ode_init(&self->o, dim, rhcpy_ode_py_thunk) == nullptr) {
               delete self;
               throw std::runtime_error("ODE init failed");
             }
             self->inited = true;
             return self;
           }),
           py::arg("method"), py::arg("dim"), py::arg("rhs"),
           "method is 'rk4' or 'euler'; rhs(t, x) returns the derivative as a length-dim array.")
      .def(
          "update", [](Ode &self, double t, Vec &x, double dt) { ode_update(&self.o, t, x.v, dt, &self); }, py::arg("t"),
          py::arg("x"), py::arg("dt"), "Advance the state vector x in place by one step of size dt.");
}

}  // namespace rhcpy
