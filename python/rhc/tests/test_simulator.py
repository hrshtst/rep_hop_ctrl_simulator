from __future__ import annotations

import gc
import math

import numpy as np

import rhc


def make_sim(rho, za=0.28, zh=0.26, zm=0.255, zb=0.24):
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.DynmorphCtrl(cmd, model, rhc.DynmorphType.SOFT_LANDING_PIECEWISE)
    cmd.set(za, zh, zm, zb)
    ctrl.set_rho(rho)
    sim = rhc.Simulator(cmd, ctrl, model)
    # cmd/model are kept alive by ctrl and sim (keep_alive).
    return ctrl, sim


def test_init_and_state():
    _, sim = make_sim(rho=1.0)
    assert sim.time == 0.0
    assert sim.step_count == 0
    sim.set_state(rhc.Vec([0.26, -0.1]))
    assert sim.z == 0.26
    assert sim.vz == -0.1
    np.testing.assert_allclose(sim.state, [0.26, -0.1])


def test_step_advances_time():
    _, sim = make_sim(rho=1.0)
    sim.set_state(rhc.Vec([0.27, 0.0]))
    sim.step(0.001)
    assert math.isclose(sim.time, 0.001)
    assert sim.step_count == 1


def test_fe_property():
    _, sim = make_sim(rho=1.0)
    sim.fe = 5.0
    assert sim.fe == 5.0


def test_rollout_shapes():
    _, sim = make_sim(rho=1.0)
    out = sim.rollout(rhc.Vec([0.27, 0.0]), 0.5, 0.001)
    n = len(out["z"])
    assert n == 500  # time / dt
    for key in ("t", "z", "vz", "fz", "phase"):
        assert len(out[key]) == n
    assert out["phase"].dtype.kind == "i"
    np.testing.assert_allclose(out["t"][:3], [0.0, 0.001, 0.002])


def test_rollout_matches_manual_stepping():
    _, sim = make_sim(rho=1.0)
    p0 = rhc.Vec([0.27, 0.0])
    dt = 0.001
    out = sim.rollout(p0, 0.5, dt)
    n = len(out["z"])

    # Reproduce by hand: the rollout must equal a manual update loop.
    sim.reset()
    sim.set_state(p0)
    z_manual = []
    vz_manual = []
    for _ in range(n):
        z_manual.append(sim.z)
        vz_manual.append(sim.vz)
        sim.update(dt)
        sim.update_time(dt)
    np.testing.assert_allclose(out["z"], z_manual)
    np.testing.assert_allclose(out["vz"], vz_manual)


def test_hopping_vs_standing():
    # rho=1 sustains repetitive hopping; rho=0 (regulator) settles without it.
    ctrl_h, sim_h = make_sim(rho=1.0)
    ctrl_s, sim_s = make_sim(rho=0.0)
    p0 = rhc.Vec([0.255, 0.5])
    out_h = sim_h.rollout(p0, 5.0, 0.001)
    sim_s.rollout(p0, 5.0, 0.001)
    # The hopper leaves the ground repeatedly and reaches above lift-off height.
    assert out_h["z"].max() > ctrl_h.zh
    assert ctrl_h.n >= 3
    # The regulator reaches far fewer apexes.
    assert ctrl_h.n > ctrl_s.n


def test_simulator_survives_owner_gc():
    # The simulator must keep cmd/ctrl/model alive after the local handles
    # that created it go out of scope (keep_alive chain).
    def build():
        cmd = rhc.Cmd()
        model = rhc.Model(10.0)
        ctrl = rhc.DynmorphCtrl(cmd, model)
        cmd.set(0.28, 0.26, 0.255, 0.24)
        ctrl.set_rho(1.0)
        return rhc.Simulator(cmd, ctrl, model)

    sim = build()
    gc.collect()
    out = sim.rollout(rhc.Vec([0.255, 0.5]), 1.0, 0.001)  # would crash on use-after-free
    assert len(out["z"]) == 1000
