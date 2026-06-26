#include <stdexcept>

#include "register.hpp"
#include "wrappers.hpp"

namespace py = pybind11;

namespace rhcpy {

void register_ppp(py::module_ &m) {
  py::class_<PPP>(m, "PhasePortraitPlotter",
                  "Runs trajectories from a grid of seeded initial conditions, logging them to a "
                  "file. For an in-memory phase portrait prefer Simulator.rollout.")
      .def(py::init([](cmd_t &cmd, Ctrl &ctrl, model_t &model, Logger &logger) {
             auto *self = new PPP();
             ppp_init(&self->p, &cmd, &ctrl.c, &model, &logger.l);
             self->inited = true;
             return self;
           }),
           py::arg("cmd"), py::arg("ctrl"), py::arg("model"), py::arg("logger"), py::keep_alive<1, 2>(),
           py::keep_alive<1, 3>(), py::keep_alive<1, 4>(), py::keep_alive<1, 5>())
      .def(
          "set_lim_xy",
          [](PPP &s, double xmin, double xmax, double ymin, double ymax) {
            ppp_set_lim_xy(&s.p, xmin, xmax, ymin, ymax);
          },
          py::arg("xmin"), py::arg("xmax"), py::arg("ymin"), py::arg("ymax"))
      .def(
          "set_n_sc_xy", [](PPP &s, int nx, int ny) { ppp_set_n_sc_xy(&s.p, nx, ny); }, py::arg("n_x"), py::arg("n_y"),
          "Number of seed samples along each axis.")
      .def(
          "push_p0", [](PPP &s, Vec &p0) { ppp_push_p0(&s.p, p0.v); }, py::arg("p0"),
          "Add an initial condition to integrate.")
      .def("generate_edge_points", [](PPP &s) { ppp_generate_edge_points(&s.p); },
           "Seed initial conditions around the region boundary.")
      .def(
          "run", [](PPP &s, double max_time, double dt) { ppp_run(&s.p, max_time, dt); }, py::arg("max_time"),
          py::arg("dt"), "Integrate every seeded trajectory, logging to the file.");
}

}  // namespace rhcpy
