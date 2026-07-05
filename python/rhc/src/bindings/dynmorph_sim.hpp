// DynmorphSim: the single high-level facade the demo app talks to.
//
// One object owns the whole coupled C system (cmd + model + dynmorph
// controller + simulator), so Python never has to juggle C-object
// lifetimes or keep_alive edges. All stepping is batched: one Python
// call advances many integration substeps with the GIL released, and
// the per-substep samples come back as column vectors that are handed
// to NumPy without copying (the arrays take ownership of the buffers).
#pragma once

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "rhc_api.hpp"

namespace rhcpy {

// Column-oriented per-substep samples covering the full CSV schema
// written by simulator_writer_default + ctrl_dynmorph_writer (except the
// leading string `tag`, which the caller supplies when writing a file).
struct Records {
  std::vector<double> t, z, vz, m, az, fe, fz;
  std::vector<double> za, zh, zm, zb, vh, phi;
  std::vector<int> n, phase;
  std::vector<double> ap_t, ap_z, ap_v, td_t, td_z, td_v;
  std::vector<double> bt_t, bt_z, bt_v, lo_t, lo_z, lo_v;
  std::vector<int> type, soft_landing;
  std::vector<double> rho, k, q_scale, q1, q2, vm;
  std::vector<double> p_za, p_zh, p_zm, p_zb, p_rho;
};

// A single (z, vz) solution curve for the phase portrait.
using Curve = std::pair<std::vector<double>, std::vector<double>>;

// Bounding box (zmin, zmax, vzmin, vzmax) for curve rollouts.
using Region = std::array<double, 4>;

class DynmorphSim {
 public:
  explicit DynmorphSim(double mass = 10.0, enum ctrl_dynmorph_types type = dynmorph_default) {
    model_init(&model_, mass);
    if (ctrl_dynmorph_create_with_type(&ctrl_, &cmd_, &model_, type) == nullptr) {
      throw std::bad_alloc();
    }
    ctrl_created_ = true;
    simulator_init(&sim_, &cmd_, &ctrl_, &model_);
    sim_inited_ = true;
    reset(stand_start_z(), 0.0);
  }

  ~DynmorphSim() {
    if (sim_inited_) {
      simulator_destroy(&sim_);
    }
    if (ctrl_created_) {
      ctrl_destroy(&ctrl_);
    }
    model_destroy(&model_);
    cmd_destroy(&cmd_);
  }

  DynmorphSim(const DynmorphSim &) = delete;
  DynmorphSim &operator=(const DynmorphSim &) = delete;
  DynmorphSim(DynmorphSim &&) = delete;
  DynmorphSim &operator=(DynmorphSim &&) = delete;

  // The paper's stand start: midway between lift-off and standing height.
  [[nodiscard]] double stand_start_z() const { return 0.5 * (cmd_.zh + cmd_.zm); }

  // Reset clock, step counter, controller events and external force, then
  // place the COM at (z, vz).
  void reset(double z, double vz) {
    simulator_reset(&sim_, nullptr);
    simulator_set_fe(&sim_, 0.0);
    set_state(z, vz);
  }

  void set_state(double z, double vz) {
    vec_set_elem(simulator_state(&sim_), 0, z);
    vec_set_elem(simulator_state(&sim_), 1, vz);
  }

  // Advance `n_steps` integration substeps of size dt, sampling every
  // `record_every`-th substep (before the step, matching simulator_run's
  // dump placement so consecutive calls form one continuous stream).
  // Runs entirely in C; safe to call with the GIL released.
  Records advance(int n_steps, double dt, int record_every = 1) {
    check_step_args(n_steps, dt, record_every);
    Records rec;
    const size_t n_rows = n_steps == 0 ? 0 : (static_cast<size_t>(n_steps) + record_every - 1) / record_every;
    resize(rec, n_rows);
    size_t row = 0;
    for (int i = 0; i < n_steps; ++i) {
      if (i % record_every == 0) {
        record_row(rec, row++);
      }
      simulator_update(&sim_, dt, nullptr);
      simulator_update_time(&sim_, dt);
    }
    return rec;
  }

  // Run a rollout from (z0, vz0) on a throwaway copy of this system; the
  // live simulation state is untouched.
  [[nodiscard]] Records rollout(double z0, double vz0, double duration, double dt, int record_every = 1) const {
    DynmorphSim tmp(model_mass(&model_), ctrl_dynmorph_type(&ctrl_));
    tmp.cmd_ = cmd_;
    tmp.reset(z0, vz0);
    const auto n_steps = static_cast<int>(std::llround(duration / dt));
    return tmp.advance(n_steps, dt, record_every);
  }

  // Roll out each (z0, vz0) seed with the current parameters and return
  // the (z, vz) samples, decimated by `stride`, for phase-portrait
  // solution curves. Also runs on a throwaway system. When a `region`
  // (zmin, zmax, vzmin, vzmax) is given, a curve stops as soon as its
  // state leaves it — mirroring ppp_simulator_is_out_of_region() in the
  // C phase-portrait plotter — so no work is spent evolving off-view
  // segments (the boundary itself counts as inside).
  [[nodiscard]] std::vector<Curve> solution_curves(const std::vector<std::pair<double, double>> &seeds,
                                                   double duration, double dt, int stride = 1,
                                                   const std::optional<Region> &region = std::nullopt) const {
    DynmorphSim tmp(model_mass(&model_), ctrl_dynmorph_type(&ctrl_));
    tmp.cmd_ = cmd_;
    const auto n_steps = static_cast<int>(std::llround(duration / dt));
    check_step_args(n_steps, dt, stride);
    std::vector<Curve> curves;
    curves.reserve(seeds.size());
    for (const auto &[z0, vz0] : seeds) {
      tmp.reset(z0, vz0);
      Curve c;
      c.first.reserve(n_steps / stride + 1);
      c.second.reserve(n_steps / stride + 1);
      for (int i = 0; i < n_steps; ++i) {
        const double z = vec_elem(simulator_state(&tmp.sim_), 0);
        const double vz = vec_elem(simulator_state(&tmp.sim_), 1);
        if (region && (z < (*region)[0] || z > (*region)[1] || vz < (*region)[2] || vz > (*region)[3])) {
          break;
        }
        if (i % stride == 0) {
          c.first.push_back(z);
          c.second.push_back(vz);
        }
        simulator_update(&tmp.sim_, dt, nullptr);
        simulator_update_time(&tmp.sim_, dt);
      }
      curves.push_back(std::move(c));
    }
    return curves;
  }

  // The exact CSV header the C pipeline writes (including leading "tag").
  [[nodiscard]] std::string csv_header() {
    char *buf = nullptr;
    size_t len = 0;
    FILE *fp = open_memstream(&buf, &len);
    if (fp == nullptr) {
      throw std::bad_alloc();
    }
    simulator_header_default(fp, &sim_, nullptr);
    std::fclose(fp);
    std::string header(buf, len);
    std::free(buf);
    return header;
  }

  // -- scalar readouts ---------------------------------------------------
  [[nodiscard]] double t() const { return simulator_time(&sim_); }
  [[nodiscard]] double z() const { return vec_elem(simulator_state(&sim_), 0); }
  [[nodiscard]] double vz() const { return vec_elem(simulator_state(&sim_), 1); }
  [[nodiscard]] double fz() const { return ctrl_fz(&ctrl_); }
  [[nodiscard]] double fe() const { return simulator_fe(&sim_); }
  void set_fe(double v) { simulator_set_fe(&sim_, v); }
  [[nodiscard]] int phase() const { return static_cast<int>(ctrl_phase(&ctrl_)); }
  [[nodiscard]] bool contact() const { return ctrl_phase_in(&ctrl_, contact); }
  [[nodiscard]] int hops() const { return ctrl_n(&ctrl_); }
  [[nodiscard]] double mass() const { return model_mass(&model_); }
  void set_mass(double v) { model_set_mass(&model_, v); }
  [[nodiscard]] double gravity() const { return model_gravity(&model_); }

  // -- commanded parameters (the tilde values the user sets) --------------
  [[nodiscard]] double za() const { return cmd_.za; }
  void set_za(double v) { cmd_.za = v; }
  [[nodiscard]] double zh() const { return cmd_.zh; }
  void set_zh(double v) { cmd_.zh = v; }
  [[nodiscard]] double zm() const { return cmd_.zm; }
  void set_zm(double v) { cmd_.zm = v; }
  [[nodiscard]] double zb() const { return cmd_.zb; }
  void set_zb(double v) { cmd_.zb = v; }
  [[nodiscard]] double rho() const { return cmd_.dynmorph.rho; }
  void set_rho(double v) { cmd_.dynmorph.rho = v; }
  [[nodiscard]] double k() const { return cmd_.dynmorph.k; }
  void set_k(double v) { cmd_.dynmorph.k = v; }
  [[nodiscard]] double q_scale() const { return cmd_.dynmorph.q_scale; }
  void set_q_scale(double v) { cmd_.dynmorph.q_scale = v; }
  [[nodiscard]] bool soft_landing() const { return cmd_.dynmorph.soft_landing; }
  void set_soft_landing(bool on) { cmd_.dynmorph.soft_landing = on; }

  // -- morphed parameters (after the adjustment layer) ---------------------
  [[nodiscard]] double p_za() const { return ctrl_dynmorph_params_za(&ctrl_); }
  [[nodiscard]] double p_zh() const { return ctrl_dynmorph_params_zh(&ctrl_); }
  [[nodiscard]] double p_zm() const { return ctrl_dynmorph_params_zm(&ctrl_); }
  [[nodiscard]] double p_zb() const { return ctrl_dynmorph_params_zb(&ctrl_); }
  [[nodiscard]] double p_rho() const { return ctrl_dynmorph_params_rho(&ctrl_); }

 private:
  static void check_step_args(int n_steps, double dt, int record_every) {
    if (dt <= 0.0) {
      throw std::invalid_argument("dt must be positive");
    }
    if (n_steps < 0) {
      throw std::invalid_argument("step count must be non-negative");
    }
    if (record_every < 1) {
      throw std::invalid_argument("record_every must be >= 1");
    }
  }

  static void resize(Records &rec, size_t n_rows) {
    for (auto *col : {&rec.t, &rec.z, &rec.vz, &rec.m, &rec.az, &rec.fe, &rec.fz, &rec.za, &rec.zh, &rec.zm,
                      &rec.zb, &rec.vh, &rec.phi, &rec.ap_t, &rec.ap_z, &rec.ap_v, &rec.td_t, &rec.td_z, &rec.td_v,
                      &rec.bt_t, &rec.bt_z, &rec.bt_v, &rec.lo_t, &rec.lo_z, &rec.lo_v, &rec.rho, &rec.k,
                      &rec.q_scale, &rec.q1, &rec.q2, &rec.vm, &rec.p_za, &rec.p_zh, &rec.p_zm, &rec.p_zb,
                      &rec.p_rho}) {
      col->resize(n_rows);
    }
    for (auto *col : {&rec.n, &rec.phase, &rec.type, &rec.soft_landing}) {
      col->resize(n_rows);
    }
  }

  void record_row(Records &rec, size_t row) {
    rec.t[row] = simulator_time(&sim_);
    rec.z[row] = vec_elem(simulator_state(&sim_), 0);
    rec.vz[row] = vec_elem(simulator_state(&sim_), 1);
    rec.m[row] = model_mass(&model_);
    rec.az[row] = model_acc(&model_);
    rec.fe[row] = simulator_fe(&sim_);
    rec.fz[row] = ctrl_fz(&ctrl_);
    rec.za[row] = cmd_.za;
    rec.zh[row] = cmd_.zh;
    rec.zm[row] = cmd_.zm;
    rec.zb[row] = cmd_.zb;
    rec.vh[row] = ctrl_vh(&ctrl_);
    rec.n[row] = ctrl_n(&ctrl_);
    rec.phi[row] = ctrl_phi(&ctrl_);
    rec.phase[row] = static_cast<int>(ctrl_phase(&ctrl_));
    rec.ap_t[row] = ctrl_events_at(&ctrl_, apex)->t;
    rec.ap_z[row] = ctrl_events_at(&ctrl_, apex)->z;
    rec.ap_v[row] = ctrl_events_at(&ctrl_, apex)->v;
    rec.td_t[row] = ctrl_events_at(&ctrl_, touchdown)->t;
    rec.td_z[row] = ctrl_events_at(&ctrl_, touchdown)->z;
    rec.td_v[row] = ctrl_events_at(&ctrl_, touchdown)->v;
    rec.bt_t[row] = ctrl_events_at(&ctrl_, bottom)->t;
    rec.bt_z[row] = ctrl_events_at(&ctrl_, bottom)->z;
    rec.bt_v[row] = ctrl_events_at(&ctrl_, bottom)->v;
    rec.lo_t[row] = ctrl_events_at(&ctrl_, liftoff)->t;
    rec.lo_z[row] = ctrl_events_at(&ctrl_, liftoff)->z;
    rec.lo_v[row] = ctrl_events_at(&ctrl_, liftoff)->v;
    rec.type[row] = static_cast<int>(ctrl_dynmorph_type(&ctrl_));
    rec.rho[row] = cmd_.dynmorph.rho;
    rec.k[row] = cmd_.dynmorph.k;
    rec.q_scale[row] = cmd_.dynmorph.q_scale;
    rec.soft_landing[row] = cmd_.dynmorph.soft_landing ? 1 : 0;
    rec.q1[row] = ctrl_dynmorph_q1(&ctrl_);
    rec.q2[row] = ctrl_dynmorph_q2(&ctrl_);
    rec.vm[row] = ctrl_dynmorph_vm(&ctrl_);
    rec.p_za[row] = p_za();
    rec.p_zh[row] = p_zh();
    rec.p_zm[row] = p_zm();
    rec.p_zb[row] = p_zb();
    rec.p_rho[row] = p_rho();
  }

  cmd_t cmd_{};
  model_t model_{};
  mutable ctrl_t ctrl_{};
  mutable simulator_t sim_{};
  bool ctrl_created_ = false;
  bool sim_inited_ = false;
};

}  // namespace rhcpy
