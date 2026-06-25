from __future__ import annotations

import math

import rhc


def test_init_and_fields():
    c = rhc.Complex(3.0, 4.0)
    assert c.re == 3.0
    assert c.im == 4.0


def test_abs():
    assert rhc.Complex(3.0, 4.0).abs() == 5.0


def test_arg():
    assert math.isclose(rhc.Complex(0.0, 1.0).arg(), math.pi / 2)
    assert math.isclose(rhc.Complex(1.0, 0.0).arg(), 0.0)


def test_polar():
    c = rhc.Complex.polar(2.0, math.pi / 2)
    assert math.isclose(c.re, 0.0, abs_tol=1e-12)
    assert math.isclose(c.im, 2.0)
