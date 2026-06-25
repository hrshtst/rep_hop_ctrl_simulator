from __future__ import annotations

import math

import rhc
from rhc_demo.driver import Driver, Params


def test_steps_advance_time():
    d = Driver(Params(rho=1.0), dt=0.001)
    d.step(10)
    assert math.isclose(d.snapshot().t, 0.010, abs_tol=1e-9)


def test_reset_returns_to_initial_state():
    d = Driver(Params(rho=1.0), dt=0.001, z0=0.255, vz0=0.0)
    d.step(50)
    d.reset()
    snap = d.snapshot()
    assert snap.t == 0.0
    assert math.isclose(snap.z, 0.255)


def test_snapshot_fields():
    d = Driver(Params(rho=1.0), dt=0.001)
    d.step(5)
    snap = d.snapshot()
    assert isinstance(snap.phase, rhc.Phase)
    assert isinstance(snap.contact, bool)
    assert snap.contact == (not d.is_in_flight())


def test_set_rho_live_changes_hopping():
    # Driven for a while, rho=1 reaches more apexes than rho=0.
    hop = Driver(Params(rho=1.0), dt=0.001, z0=0.255, vz0=0.5)
    stand = Driver(Params(rho=0.0), dt=0.001, z0=0.255, vz0=0.5)
    hop.step(5000)
    stand.step(5000)
    assert hop.snapshot().n > stand.snapshot().n
    assert hop.snapshot().n >= 3


def test_live_setters_mutate_running_system():
    d = Driver(Params(rho=0.0), dt=0.001)
    d.set_rho(1.0)
    d.set_za(0.30)
    d.set_mass(12.0)
    assert d.params.rho == 1.0
    assert d.params.za == 0.30
    assert d.params.mass == 12.0


def test_disturbance_sets_then_clears_fe():
    d = Driver(Params(rho=1.0), dt=0.001)
    d.apply_disturbance(50.0, duration=0.003)
    d.step(1)
    assert d.sim_fe() == 50.0
    d.step(5)  # exceeds the 0.003 s window
    assert d.sim_fe() == 0.0
