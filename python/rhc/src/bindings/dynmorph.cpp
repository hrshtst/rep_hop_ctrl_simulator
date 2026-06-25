#include <stdexcept>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_dynmorph(py::module_ &m) {
  py::enum_<enum ctrl_dynmorph_types>(m, "DynmorphType", "Parameter-adjustment strategy of the dynmorph controller.")
      .value("DYNMORPH_DEFAULT", dynmorph_default)
      .value("SOFT_LANDING_PIECEWISE", soft_landing_piecewise)
      .value("SOFT_LANDING_LINEAR", soft_landing_linear)
      .value("FIX_ZB", fix_zb)
      .value("FIX_ZM", fix_zm)
      .export_values();

  py::class_<DynmorphCtrl, Ctrl>(m, "DynmorphCtrl", "The dynamics-morphing standing/hopping controller.")
      .def(py::init([](cmd_t &cmd, model_t &model, enum ctrl_dynmorph_types type) {
             auto *self = new DynmorphCtrl();
             if (ctrl_dynmorph_create_with_type(&self->c, &cmd, &model, type) == nullptr) {
               delete self;
               throw std::runtime_error("failed to create dynmorph controller");
             }
             self->created = true;
             return self;
           }),
           py::arg("cmd"), py::arg("model"), py::arg("type") = dynmorph_default, py::keep_alive<1, 2>(),
           py::keep_alive<1, 3>(), "Create a dynmorph controller bound to the given cmd and model.")
      .def_property_readonly("type", [](DynmorphCtrl &self) { return ctrl_dynmorph_type(&self.c); })
      .def_property(
          "rho", [](DynmorphCtrl &self) { return ctrl_dynmorph_rho(&self.c); },
          [](DynmorphCtrl &self, double v) { ctrl_dynmorph_set_rho(&self.c, v); },
          "Morphing parameter: 0 = standing regulator, 1 = hopping oscillator.")
      .def_property(
          "k", [](DynmorphCtrl &self) { return ctrl_dynmorph_k(&self.c); },
          [](DynmorphCtrl &self, double v) { ctrl_dynmorph_set_k(&self.c, v); }, "Convergence gain.")
      .def_property_readonly("soft_landing",
                             [](DynmorphCtrl &self) { return static_cast<bool>(ctrl_dynmorph_soft_landing(&self.c)); })
      .def(
          "set_rho", [](DynmorphCtrl &self, double v) { ctrl_dynmorph_set_rho(&self.c, v); }, py::arg("rho"))
      .def(
          "set_k", [](DynmorphCtrl &self, double v) { ctrl_dynmorph_set_k(&self.c, v); }, py::arg("k"))
      .def("enable_soft_landing", [](DynmorphCtrl &self) { ctrl_dynmorph_enable_soft_landing(&self.c); })
      .def("disable_soft_landing", [](DynmorphCtrl &self) { ctrl_dynmorph_disable_soft_landing(&self.c); })
      // The instantaneous control parameters after the adjustment layer.
      .def_property_readonly("param_za", [](DynmorphCtrl &self) { return ctrl_dynmorph_params_za(&self.c); })
      .def_property_readonly("param_zh", [](DynmorphCtrl &self) { return ctrl_dynmorph_params_zh(&self.c); })
      .def_property_readonly("param_zm", [](DynmorphCtrl &self) { return ctrl_dynmorph_params_zm(&self.c); })
      .def_property_readonly("param_zb", [](DynmorphCtrl &self) { return ctrl_dynmorph_params_zb(&self.c); })
      .def_property_readonly("param_rho", [](DynmorphCtrl &self) { return ctrl_dynmorph_params_rho(&self.c); })
      .def_static(
          "calc_za", [](double zh, double zm, double zb) { return ctrl_dynmorph_calc_za(zh, zm, zb); }, py::arg("zh"),
          py::arg("zm"), py::arg("zb"), "Target apex height from zh, zm, zb.")
      .def_static(
          "calc_zh", [](double za, double zm, double zb) { return ctrl_dynmorph_calc_zh(za, zm, zb); }, py::arg("za"),
          py::arg("zm"), py::arg("zb"))
      .def_static(
          "calc_zm", [](double za, double zh, double zb) { return ctrl_dynmorph_calc_zm(za, zh, zb); }, py::arg("za"),
          py::arg("zh"), py::arg("zb"))
      .def_static(
          "calc_zb", [](double za, double zh, double zm) { return ctrl_dynmorph_calc_zb(za, zh, zm); }, py::arg("za"),
          py::arg("zh"), py::arg("zm"), "Crouching bottom from za, zh, zm.");
}

}  // namespace rhcpy
