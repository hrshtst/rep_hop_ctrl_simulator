#include <stdexcept>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_raibert(py::module_ &m) {
  py::enum_<enum ctrl_raibert_types>(m, "RaibertType", "Variant of the Raibert hopping controller.")
      .value("NONE", none)
      .value("FULL_NONLINEAR", full_nonlinear)
      .value("SIMPLIFIED_NONLINEAR", simplified_nonlinear)
      .value("FULL_LINEAR", full_linear)
      .value("SIMPLIFIED_LINEAR", simplified_linear)
      .export_values();

  py::class_<RaibertCtrl, Ctrl>(m, "RaibertCtrl", "Raibert-style hopping controller.")
      .def(py::init([](cmd_t &cmd, model_t &model, enum ctrl_raibert_types type) {
             auto *self = new RaibertCtrl();
             if (ctrl_raibert_create(&self->c, &cmd, &model, type) == nullptr) {
               delete self;
               throw std::runtime_error("failed to create raibert controller");
             }
             self->created = true;
             return self;
           }),
           py::arg("cmd"), py::arg("model"), py::arg("type"), py::keep_alive<1, 2>(), py::keep_alive<1, 3>())
      .def_property_readonly("type", [](RaibertCtrl &s) { return ctrl_raibert_type(&s.c); })
      .def(
          "set_params",
          [](RaibertCtrl &s, double delta, double tau, double gamma, double yeta1, double zr, double mu) {
            ctrl_raibert_set_params(&s.c, delta, tau, gamma, yeta1, zr, mu);
          },
          py::arg("delta"), py::arg("tau"), py::arg("gamma"), py::arg("yeta1"), py::arg("zr"), py::arg("mu"))
      .def_property(
          "delta", [](RaibertCtrl &s) { return ctrl_raibert_delta(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_delta(&s.c, v); })
      .def_property(
          "tau", [](RaibertCtrl &s) { return ctrl_raibert_tau(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_tau(&s.c, v); })
      .def_property(
          "gamma", [](RaibertCtrl &s) { return ctrl_raibert_gamma(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_gamma(&s.c, v); })
      .def_property(
          "yeta1", [](RaibertCtrl &s) { return ctrl_raibert_yeta1(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_yeta1(&s.c, v); })
      .def_property(
          "zr", [](RaibertCtrl &s) { return ctrl_raibert_zr(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_zr(&s.c, v); })
      .def_property(
          "mu", [](RaibertCtrl &s) { return ctrl_raibert_mu(&s.c); },
          [](RaibertCtrl &s, double v) { ctrl_raibert_set_mu(&s.c, v); })
      .def("is_in_thrust", [](RaibertCtrl &s) { return static_cast<bool>(ctrl_raibert_is_in_thrust(&s.c)); });
}

void register_regulator(py::module_ &m) {
  py::class_<RegulatorCtrl, Ctrl>(m, "RegulatorCtrl", "COM-height regulator (standing) controller.")
      .def(py::init([](cmd_t &cmd, model_t &model) {
             auto *self = new RegulatorCtrl();
             if (ctrl_regulator_create(&self->c, &cmd, &model) == nullptr) {
               delete self;
               throw std::runtime_error("failed to create regulator controller");
             }
             self->created = true;
             return self;
           }),
           py::arg("cmd"), py::arg("model"), py::keep_alive<1, 2>(), py::keep_alive<1, 3>())
      .def_property(
          "q1", [](RegulatorCtrl &s) { return ctrl_regulator_q1(&s.c); },
          [](RegulatorCtrl &s, double v) { ctrl_regulator_q1(&s.c) = v; })
      .def_property(
          "q2", [](RegulatorCtrl &s) { return ctrl_regulator_q2(&s.c); },
          [](RegulatorCtrl &s, double v) { ctrl_regulator_q2(&s.c) = v; })
      .def_property_readonly("xi", [](RegulatorCtrl &s) { return ctrl_regulator_xi(&s.c); });
}

}  // namespace rhcpy
