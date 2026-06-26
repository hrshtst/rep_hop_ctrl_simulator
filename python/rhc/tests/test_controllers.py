from __future__ import annotations

import rhc


def test_regulator_create_defaults():
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.RegulatorCtrl(cmd, model)
    # ctrl_regulator_cmd_init sets q1=2, q2=3.
    assert ctrl.q1 == 2.0
    assert ctrl.q2 == 3.0


def test_regulator_update_force_and_xi():
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.RegulatorCtrl(cmd, model)
    cmd.set(0.28, 0.26, 0.255, 0.24)
    ctrl.update(0.0, rhc.Vec([0.25, 0.0]))  # stance
    assert ctrl.fz >= 0.0
    assert ctrl.xi > 0.0


def test_raibert_create_and_params():
    cmd = rhc.Cmd()
    model = rhc.Model(1.0)
    ctrl = rhc.RaibertCtrl(cmd, model, rhc.RaibertType.SIMPLIFIED_NONLINEAR)
    assert ctrl.type == rhc.RaibertType.SIMPLIFIED_NONLINEAR
    ctrl.set_params(0.0, 41.86, 0.0, 5.81, 0.0, 0.0)
    assert ctrl.tau == 41.86
    assert ctrl.yeta1 == 5.81


def test_raibert_property_setters():
    cmd = rhc.Cmd()
    model = rhc.Model(1.0)
    ctrl = rhc.RaibertCtrl(cmd, model, rhc.RaibertType.FULL_NONLINEAR)
    ctrl.delta = 0.01
    ctrl.mu = 1.0
    assert ctrl.delta == 0.01
    assert ctrl.mu == 1.0
    assert isinstance(ctrl.is_in_thrust(), bool)
