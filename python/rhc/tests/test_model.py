from __future__ import annotations

import math

import rhc

G = 9.806652  # rhc_misc.h gravitational acceleration


def test_init():
    m = rhc.Model(10.0)
    assert m.mass == 10.0
    assert m.acc == 0.0
    assert math.isclose(m.gravity, G)


def test_writable_properties():
    m = rhc.Model(10.0)
    m.mass = 12.0
    m.gravity = 9.81
    assert m.mass == 12.0
    assert m.gravity == 9.81


def test_calc_acc_clamps_negative_fz():
    # fz < 0 is clamped to 0 (the ground cannot pull the robot).
    assert rhc.Model.calc_acc(1.0, -5.0, 0.0, 10.0) == -10.0


def test_calc_acc():
    # acc = fz/m - g + fe/m
    assert rhc.Model.calc_acc(2.0, 20.0, 4.0, 10.0) == (20.0 / 2.0 - 10.0 + 4.0 / 2.0)


def test_update_sets_acc():
    m = rhc.Model(2.0)
    m.update(fz=20.0, fe=0.0)
    assert math.isclose(m.acc, 20.0 / 2.0 - G)
