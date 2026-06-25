from __future__ import annotations

import math

import rhc


def make_ctrl(
    ctrl_type=None,
    za=0.28,
    zh=0.26,
    zm=0.255,
    zb=0.24,
    rho=1.0,
    *,
    soft_landing=True,
):
    if ctrl_type is None:
        ctrl_type = rhc.DynmorphType.SOFT_LANDING_PIECEWISE
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.DynmorphCtrl(cmd, model, ctrl_type)
    cmd.set(za, zh, zm, zb)
    ctrl.set_rho(rho)
    if soft_landing:
        ctrl.enable_soft_landing()
    else:
        ctrl.disable_soft_landing()
    # cmd and model are kept alive by the controller (pybind11 keep_alive),
    # so returning only the controller is safe.
    return ctrl


def test_create_applies_cmd_defaults():
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.DynmorphCtrl(cmd, model)
    # create_with_type runs cmd_init: default heights, rho=0, k=4, soft landing on.
    assert ctrl.type == rhc.DynmorphType.DYNMORPH_DEFAULT
    assert ctrl.rho == 0.0
    assert ctrl.k == 4.0
    assert ctrl.soft_landing is True
    assert cmd.za == 0.28


def test_set_rho_and_k():
    ctrl = make_ctrl(rho=0.5)
    assert ctrl.rho == 0.5
    ctrl.set_k(6.0)
    assert ctrl.k == 6.0


def test_soft_landing_toggle():
    ctrl = make_ctrl()
    ctrl.disable_soft_landing()
    assert ctrl.soft_landing is False
    ctrl.enable_soft_landing()
    assert ctrl.soft_landing is True


def test_height_relations_roundtrip():
    zb = rhc.DynmorphCtrl.calc_zb(0.28, 0.26, 0.255)
    assert math.isclose(zb, 0.24, abs_tol=1e-9)
    za = rhc.DynmorphCtrl.calc_za(0.26, 0.255, zb)
    assert math.isclose(za, 0.28, abs_tol=1e-9)


def test_flight_has_zero_fz():
    ctrl = make_ctrl(rho=1.0)
    p = rhc.Vec([0.30, -0.1])  # z = 0.30 > zh = 0.26 -> flight
    ctrl.update(0.0, p)
    assert ctrl.fz == 0.0
    assert ctrl.is_in_flight() is True
    assert ctrl.is_in_contact() is False


def test_regulator_vs_oscillator_differ_in_stance():
    # In stance with non-zero velocity, rho=0 (regulator) and rho=1
    # (oscillator) produce different, non-negative ground reaction forces.
    c0 = make_ctrl(rho=0.0, soft_landing=False)
    c1 = make_ctrl(rho=1.0, soft_landing=False)
    p = rhc.Vec([0.25, 0.1])  # z = 0.25 < zh -> stance, vz != 0
    c0.update(0.0, p)
    c1.update(0.0, p)
    assert c0.fz >= 0.0
    assert c1.fz >= 0.0
    assert not math.isclose(c0.fz, c1.fz)


def test_phase_and_event_access():
    ctrl = make_ctrl(rho=1.0)
    ctrl.update(0.0, rhc.Vec([0.28, 0.0]))
    assert ctrl.phase in (
        rhc.Phase.FALLING,
        rhc.Phase.RISING,
        rhc.Phase.COMPRESSION,
        rhc.Phase.EXTENSION,
    )
    apex = ctrl.apex
    assert hasattr(apex, "t")
    assert hasattr(apex, "z")
    assert hasattr(apex, "v")
