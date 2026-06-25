#include <pybind11/numpy.h>

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

namespace {

// Run an in-memory rollout and return the recorded time series as a dict of
// NumPy arrays. No file I/O, so it is suitable for live use.
py::dict simulator_rollout_to_dict(Simulator &sim, Vec &p0, double time, double dt) {
  if (dt <= 0.0) {
    throw std::invalid_argument("dt must be positive");
  }
  if (time < 0.0) {
    throw std::invalid_argument("time must be non-negative");
  }
  auto cap = static_cast<py::ssize_t>(std::ceil(time / dt)) + 1;
  if (cap < 1) {
    cap = 1;
  }

  auto t = py::array_t<double>(cap);
  auto z = py::array_t<double>(cap);
  auto vz = py::array_t<double>(cap);
  auto fz = py::array_t<double>(cap);
  auto phase = py::array_t<int>(cap);

  const int n = simulator_rollout(&sim.s, p0.v, time, dt, t.mutable_data(), z.mutable_data(), vz.mutable_data(),
                                  fz.mutable_data(), phase.mutable_data(), static_cast<int>(cap), nullptr);

  const auto count = static_cast<py::ssize_t>(n);
  t.resize({count});
  z.resize({count});
  vz.resize({count});
  fz.resize({count});
  phase.resize({count});

  py::dict out;
  out["t"] = t;
  out["z"] = z;
  out["vz"] = vz;
  out["fz"] = fz;
  out["phase"] = phase;
  return out;
}

}  // namespace

void register_simulator(py::module_ &m) {
  py::class_<Simulator>(m, "Simulator", "Drives a controller + model through the RK4 integrator.")
      .def(py::init([](cmd_t &cmd, Ctrl &ctrl, model_t &model) {
             auto *self = new Simulator();
             simulator_init(&self->s, &cmd, &ctrl.c, &model);
             self->inited = true;
             return self;
           }),
           py::arg("cmd"), py::arg("ctrl"), py::arg("model"), py::keep_alive<1, 2>(), py::keep_alive<1, 3>(),
           py::keep_alive<1, 4>(), "Create a simulator bound to the given cmd, controller and model.")
      .def("reset", [](Simulator &self) { return static_cast<bool>(simulator_reset(&self.s, nullptr)); },
           "Reset time, step and the controller. Returns False if the reset failed.")
      .def(
          "set_state", [](Simulator &self, Vec &p) { simulator_set_state(&self.s, p.v); }, py::arg("state"),
          "Set the state [z, vz].")
      .def(
          "update", [](Simulator &self, double dt) { return static_cast<bool>(simulator_update(&self.s, dt, nullptr)); },
          py::arg("dt"), "Advance the state by one RK4 step (does not advance the clock).")
      .def(
          "update_time", [](Simulator &self, double dt) { simulator_update_time(&self.s, dt); }, py::arg("dt"),
          "Advance the step counter and simulation time.")
      .def(
          "step",
          [](Simulator &self, double dt) {
            const bool ok = simulator_update(&self.s, dt, nullptr);
            simulator_update_time(&self.s, dt);
            return ok;
          },
          py::arg("dt"), "Advance one full step: integrate then advance the clock.")
      .def(
          "run",
          [](Simulator &self, Vec &p0, double time, double dt) {
            simulator_run(&self.s, p0.v, time, dt, nullptr, nullptr);
          },
          py::arg("p0"), py::arg("time"), py::arg("dt"), "Run a full simulation from p0 (no logging).")
      .def("rollout", &simulator_rollout_to_dict, py::arg("p0"), py::arg("time"), py::arg("dt"),
           "Run from p0 and return {t, z, vz, fz, phase} as NumPy arrays, entirely in memory.")
      .def_property_readonly("time", [](Simulator &self) { return simulator_time(&self.s); })
      .def_property_readonly("step_count", [](Simulator &self) { return simulator_step(&self.s); })
      .def_property_readonly("z", [](Simulator &self) { return vec_elem(simulator_state(&self.s), 0); })
      .def_property_readonly("vz", [](Simulator &self) { return vec_elem(simulator_state(&self.s), 1); })
      .def_property_readonly("state",
                             [](Simulator &self) {
                               vec_t st = simulator_state(&self.s);
                               auto a = py::array_t<double>(static_cast<py::ssize_t>(vec_size(st)));
                               std::memcpy(a.mutable_data(), vec_buf(st), vec_size(st) * sizeof(double));
                               return a;
                             })
      .def_property(
          "fe", [](Simulator &self) { return simulator_fe(&self.s); },
          [](Simulator &self, double v) { simulator_set_fe(&self.s, v); }, "External vertical force.");
}

}  // namespace rhcpy
