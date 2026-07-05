// Pybind11 module: exposes DynmorphSim plus the phase / controller-type
// enums. Long-running calls (advance, rollout, solution_curves) release
// the GIL while the C core runs, and their sample columns are moved into
// NumPy arrays without copying.
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <utility>
#include <vector>

#include "dynmorph_sim.hpp"

namespace py = pybind11;

namespace rhcpy {
namespace {

// Hand a C++ vector to NumPy without copying: the array owns the vector
// through its base capsule and frees it when garbage-collected.
template <typename T>
py::array_t<T> take(std::vector<T> &&v) {
  auto *held = new std::vector<T>(std::move(v));
  py::capsule owner(held, [](void *p) { delete static_cast<std::vector<T> *>(p); });
  return py::array_t<T>({static_cast<py::ssize_t>(held->size())}, {sizeof(T)}, held->data(), owner);
}

py::dict to_dict(Records &&r) {
  py::dict d;
  d["t"] = take(std::move(r.t));
  d["z"] = take(std::move(r.z));
  d["vz"] = take(std::move(r.vz));
  d["m"] = take(std::move(r.m));
  d["az"] = take(std::move(r.az));
  d["fe"] = take(std::move(r.fe));
  d["fz"] = take(std::move(r.fz));
  d["za"] = take(std::move(r.za));
  d["zh"] = take(std::move(r.zh));
  d["zm"] = take(std::move(r.zm));
  d["zb"] = take(std::move(r.zb));
  d["vh"] = take(std::move(r.vh));
  d["n"] = take(std::move(r.n));
  d["phi"] = take(std::move(r.phi));
  d["phase"] = take(std::move(r.phase));
  d["ap_t"] = take(std::move(r.ap_t));
  d["ap_z"] = take(std::move(r.ap_z));
  d["ap_v"] = take(std::move(r.ap_v));
  d["td_t"] = take(std::move(r.td_t));
  d["td_z"] = take(std::move(r.td_z));
  d["td_v"] = take(std::move(r.td_v));
  d["bt_t"] = take(std::move(r.bt_t));
  d["bt_z"] = take(std::move(r.bt_z));
  d["bt_v"] = take(std::move(r.bt_v));
  d["lo_t"] = take(std::move(r.lo_t));
  d["lo_z"] = take(std::move(r.lo_z));
  d["lo_v"] = take(std::move(r.lo_v));
  d["type"] = take(std::move(r.type));
  d["rho"] = take(std::move(r.rho));
  d["k"] = take(std::move(r.k));
  d["q_scale"] = take(std::move(r.q_scale));
  d["soft_landing"] = take(std::move(r.soft_landing));
  d["q1"] = take(std::move(r.q1));
  d["q2"] = take(std::move(r.q2));
  d["vm"] = take(std::move(r.vm));
  d["p_za"] = take(std::move(r.p_za));
  d["p_zh"] = take(std::move(r.p_zh));
  d["p_zm"] = take(std::move(r.p_zm));
  d["p_zb"] = take(std::move(r.p_zb));
  d["p_rho"] = take(std::move(r.p_rho));
  return d;
}

py::list curves_to_list(std::vector<Curve> &&curves) {
  py::list out;
  for (auto &c : curves) {
    out.append(py::make_tuple(take(std::move(c.first)), take(std::move(c.second))));
  }
  return out;
}

}  // namespace

PYBIND11_MODULE(_rhc, m) {
  m.doc() = "Python bindings for the rep_hop_ctrl_simulator C library (rhc).";
  m.attr("__version__") = "0.2.0";

  py::enum_<_ctrl_events_phases_t>(m, "Phase", "Controller phase within a hop cycle.")
      .value("INVALID", invalid)
      .value("FALLING", falling)
      .value("COMPRESSION", compression)
      .value("EXTENSION", extension)
      .value("RISING", rising);

  py::enum_<enum ctrl_dynmorph_types>(m, "DynmorphType", "Parameter-adjustment strategy of the dynmorph controller.")
      .value("DEFAULT", dynmorph_default)
      .value("SOFT_LANDING_PIECEWISE", soft_landing_piecewise)
      .value("SOFT_LANDING_LINEAR", soft_landing_linear)
      .value("FIX_ZB", fix_zb)
      .value("FIX_ZM", fix_zm);

  py::class_<DynmorphSim>(m, "DynmorphSim",
                          "Vertical-COM hopping simulation driven by the dynamics-morphing controller.\n"
                          "Owns the whole C system (cmd + model + controller + integrator); defaults\n"
                          "match the paper (za=0.28, zh=0.26, zm=0.255, zb=0.23 m, mass=10 kg, rho=0).")
      .def(py::init<double, enum ctrl_dynmorph_types>(), py::arg("mass") = 10.0,
           py::arg("type") = dynmorph_default)
      .def("reset", &DynmorphSim::reset, py::arg("z"), py::arg("vz") = 0.0,
           "Reset clock, controller events and external force; place the COM at (z, vz).")
      .def(
          "reset", [](DynmorphSim &self) { self.reset(self.stand_start_z(), 0.0); },
          "Reset to the paper's stand start ((zh+zm)/2, 0).")
      .def("set_state", &DynmorphSim::set_state, py::arg("z"), py::arg("vz"),
           "Overwrite the COM state without resetting the controller.")
      .def(
          "advance",
          [](DynmorphSim &self, int n_steps, double dt, int record_every) {
            Records rec;
            {
              py::gil_scoped_release release;
              rec = self.advance(n_steps, dt, record_every);
            }
            return to_dict(std::move(rec));
          },
          py::arg("n_steps"), py::arg("dt") = 1e-4, py::arg("record_every") = 1,
          "Advance n_steps substeps of size dt; returns the sampled columns as NumPy arrays.")
      .def(
          "rollout",
          [](const DynmorphSim &self, double z0, double vz0, double duration, double dt, int record_every) {
            Records rec;
            {
              py::gil_scoped_release release;
              rec = self.rollout(z0, vz0, duration, dt, record_every);
            }
            return to_dict(std::move(rec));
          },
          py::arg("z0"), py::arg("vz0"), py::arg("duration"), py::arg("dt") = 1e-4, py::arg("record_every") = 1,
          "Run a rollout from (z0, vz0) on a throwaway copy; the live state is untouched.")
      .def(
          "solution_curves",
          [](const DynmorphSim &self, const std::vector<std::pair<double, double>> &seeds, double duration,
             double dt, int stride) {
            std::vector<Curve> curves;
            {
              py::gil_scoped_release release;
              curves = self.solution_curves(seeds, duration, dt, stride);
            }
            return curves_to_list(std::move(curves));
          },
          py::arg("seeds"), py::arg("duration") = 1.0, py::arg("dt") = 4e-4, py::arg("stride") = 5,
          "Phase-portrait solution curves: one (z, vz) array pair per seed.")
      .def("csv_header", &DynmorphSim::csv_header,
           "The exact CSV header line the C pipeline writes (starts with 'tag').")
      .def_property_readonly("stand_start_z", &DynmorphSim::stand_start_z)
      .def_property_readonly("t", &DynmorphSim::t)
      .def_property_readonly("z", &DynmorphSim::z)
      .def_property_readonly("vz", &DynmorphSim::vz)
      .def_property_readonly("fz", &DynmorphSim::fz, "Ground reaction force from the controller.")
      .def_property("fe", &DynmorphSim::fe, &DynmorphSim::set_fe, "External vertical force (scalar; 1-DOF plant).")
      .def_property_readonly("phase", [](const DynmorphSim &self) { return static_cast<_ctrl_events_phases_t>(self.phase()); })
      .def_property_readonly("contact", &DynmorphSim::contact)
      .def_property_readonly("hops", &DynmorphSim::hops, "Number of apexes reached so far.")
      .def_property("mass", &DynmorphSim::mass, &DynmorphSim::set_mass)
      .def_property_readonly("gravity", &DynmorphSim::gravity)
      .def_property("za", &DynmorphSim::za, &DynmorphSim::set_za, "Commanded apex height (tilde z_a).")
      .def_property("zh", &DynmorphSim::zh, &DynmorphSim::set_zh, "Lift-off height (robot constant).")
      .def_property("zm", &DynmorphSim::zm, &DynmorphSim::set_zm, "Commanded standing height (tilde z_m).")
      .def_property("zb", &DynmorphSim::zb, &DynmorphSim::set_zb, "Commanded kinematic lower limit (tilde z_b).")
      .def_property("rho", &DynmorphSim::rho, &DynmorphSim::set_rho,
                    "Morphing parameter: 0 = standing regulator, 1 = hopping oscillator.")
      .def_property("k", &DynmorphSim::k, &DynmorphSim::set_k, "Convergence gain.")
      .def_property("q_scale", &DynmorphSim::q_scale, &DynmorphSim::set_q_scale)
      .def_property("soft_landing", &DynmorphSim::soft_landing, &DynmorphSim::set_soft_landing)
      .def_property_readonly("p_za", &DynmorphSim::p_za, "Apex height after the adjustment layer.")
      .def_property_readonly("p_zh", &DynmorphSim::p_zh)
      .def_property_readonly("p_zm", &DynmorphSim::p_zm)
      .def_property_readonly("p_zb", &DynmorphSim::p_zb)
      .def_property_readonly("p_rho", &DynmorphSim::p_rho);
}

}  // namespace rhcpy
