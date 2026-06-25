#include <string>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_ctrl(py::module_ &m) {
  py::enum_<enum _ctrl_events_phases_t>(m, "Phase", "Gait phase of the hopping motion.")
      .value("INVALID", invalid)
      .value("FALLING", falling)
      .value("COMPRESSION", compression)
      .value("EXTENSION", extension)
      .value("RISING", rising)
      .export_values();

  py::class_<_ctrl_events_tuple_t>(m, "Event", "A recorded gait event: time, position and velocity.")
      .def_readonly("t", &_ctrl_events_tuple_t::t)
      .def_readonly("z", &_ctrl_events_tuple_t::z)
      .def_readonly("v", &_ctrl_events_tuple_t::v)
      .def("__repr__", [](const _ctrl_events_tuple_t &e) {
        return "Event(t=" + std::to_string(e.t) + ", z=" + std::to_string(e.z) + ", v=" + std::to_string(e.v) + ")";
      });

  py::class_<Ctrl>(m, "Ctrl", "Base controller: read-only access to the controller and gait state.")
      .def_property_readonly("fz", [](Ctrl &self) { return ctrl_fz(&self.c); }, "Ground reaction force.")
      .def_property_readonly("za", [](Ctrl &self) { return ctrl_za(&self.c); })
      .def_property_readonly("zh", [](Ctrl &self) { return ctrl_zh(&self.c); })
      .def_property_readonly("zm", [](Ctrl &self) { return ctrl_zm(&self.c); })
      .def_property_readonly("zb", [](Ctrl &self) { return ctrl_zb(&self.c); })
      .def_property_readonly("phase", [](Ctrl &self) { return ctrl_phase(&self.c); })
      .def_property_readonly("phi", [](Ctrl &self) { return ctrl_phi(&self.c); })
      .def_property_readonly("n", [](Ctrl &self) { return ctrl_n(&self.c); }, "Number of apexes reached.")
      .def_property_readonly("vh", [](Ctrl &self) { return ctrl_vh(&self.c); })
      .def_property_readonly("apex", [](Ctrl &self) { return *ctrl_events_at(&self.c, apex); })
      .def_property_readonly("touchdown", [](Ctrl &self) { return *ctrl_events_at(&self.c, touchdown); })
      .def_property_readonly("bottom", [](Ctrl &self) { return *ctrl_events_at(&self.c, bottom); })
      .def_property_readonly("liftoff", [](Ctrl &self) { return *ctrl_events_at(&self.c, liftoff); })
      .def("is_in_flight", [](Ctrl &self) { return static_cast<bool>(ctrl_events_is_in_flight(ctrl_events(&self.c))); })
      .def("is_in_contact", [](Ctrl &self) { return static_cast<bool>(ctrl_events_is_in_contact(ctrl_events(&self.c))); })
      .def(
          "update", [](Ctrl &self, double t, Vec &p) { ctrl_update(&self.c, t, p.v); }, py::arg("t"), py::arg("p"),
          "Advance the controller's gait state and force for the given time and state [z, vz].")
      .def("reset", [](Ctrl &self) { ctrl_reset(&self.c, nullptr); });
}

}  // namespace rhcpy
