from __future__ import annotations

import math

import numpy as np
import pytest

import rhc


def test_rk4_harmonic_oscillator():
    # x' = [v, -w^2 x]; with x0=[1,0] the position is cos(w t).
    omega = 2.0

    def rhs(t, x):
        del t
        return np.array([x[1], -(omega**2) * x[0]])

    ode = rhc.Ode("rk4", 2, rhs)
    x = rhc.Vec([1.0, 0.0])
    dt = 1e-3
    t = 0.0
    for _ in range(1000):  # integrate to t = 1.0
        ode.update(t, x, dt)
        t += dt
    assert math.isclose(x[0], math.cos(omega), abs_tol=1e-3)
    assert math.isclose(x[1], -omega * math.sin(omega), abs_tol=2e-3)


def test_euler_decay():
    # x' = -x  =>  x(t) = exp(-t).
    def rhs(t, x):
        del t
        return np.array([-x[0]])

    ode = rhc.Ode("euler", 1, rhs)
    x = rhc.Vec([1.0])
    dt = 1e-4
    t = 0.0
    for _ in range(10000):  # to t = 1.0
        ode.update(t, x, dt)
        t += dt
    assert math.isclose(x[0], math.exp(-1.0), abs_tol=1e-3)


def test_bad_method_raises():
    with pytest.raises(ValueError):
        rhc.Ode("midpoint", 1, lambda t, x: x)
