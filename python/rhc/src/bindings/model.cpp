#include <cstdio>
#include <string>

#include "register.hpp"
#include "rhc_api.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_model(py::module_ &m) {
  py::class_<model_t>(m, "Model", "Point-mass vertical centre-of-mass model.")
      .def(py::init([](double mass) {
             model_t mo;
             model_init(&mo, mass);
             return mo;
           }),
           py::arg("mass"))
      .def_property(
          "mass", [](model_t &s) { return model_mass(&s); },
          [](model_t &s, double v) { model_set_mass(&s, v); })
      .def_property(
          "acc", [](model_t &s) { return model_acc(&s); }, [](model_t &s, double v) { model_set_acc(&s, v); },
          "Most recently computed acceleration.")
      .def_property(
          "gravity", [](model_t &s) { return model_gravity(&s); },
          [](model_t &s, double v) { model_set_gravity(&s, v); })
      .def(
          "update", [](model_t &s, double fz, double fe) { model_update(&s, fz, fe); }, py::arg("fz"), py::arg("fe"),
          "Recompute and store acc from ground reaction force fz and external force fe.")
      .def_static(
          "calc_acc", [](double m, double fz, double fe, double g) { return model_calc_acc(m, fz, fe, g); },
          py::arg("m"), py::arg("fz"), py::arg("fe"), py::arg("g"),
          "Acceleration fz/m - g + fe/m, with fz clamped to be non-negative.")
      .def("__repr__", [](model_t &s) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "Model(mass=%g, gravity=%g, acc=%g)", s.m, s.gravity, s.acc);
        return std::string(buf);
      });
}

}  // namespace rhcpy
