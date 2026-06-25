#include <string>

#include "register.hpp"
#include "rhc_api.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_complex(py::module_ &m) {
  py::class_<complex_t>(m, "Complex", "A complex number with real and imaginary parts.")
      .def(py::init([](double re, double im) {
             complex_t c;
             complex_init(&c, re, im);
             return c;
           }),
           py::arg("re"), py::arg("im"))
      .def_readwrite("re", &complex_t::re)
      .def_readwrite("im", &complex_t::im)
      .def("abs", [](complex_t &c) { return complex_abs(&c); }, "Magnitude sqrt(re^2 + im^2).")
      .def("arg", [](complex_t &c) { return complex_arg(&c); }, "Argument atan2(im, re).")
      .def_static(
          "polar",
          [](double r, double theta) {
            complex_t c;
            complex_polar(&c, r, theta);
            return c;
          },
          py::arg("r"), py::arg("theta"), "Construct from polar coordinates.")
      .def("__repr__", [](complex_t &c) {
        return "Complex(" + std::to_string(c.re) + ", " + std::to_string(c.im) + ")";
      });
}

}  // namespace rhcpy
