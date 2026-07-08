"""Control parameters, paper defaults, and kinematic-constraint clamping.

The slider constraints from the directive are enforced here as pure
functions so they can be unit-tested without Qt:
``zb < zm < zh`` and ``zb < za`` must never be violated.
"""

from __future__ import annotations

from dataclasses import dataclass, replace

# Minimum separation kept between constrained heights, in metres.
EPS = 1e-3

# Absolute slider ranges, in metres. zh is a robot constant (not a slider).
ZA_RANGE = (0.20, 0.40)
ZM_RANGE = (0.20, 0.30)
ZB_RANGE = (0.18, 0.30)
K_RANGE = (0.5, 16.0)
Q_SCALE_RANGE = (0.5, 2.0)

# Phase-portrait view region, shared by the phase view (its world window)
# and the solution-curve seeding (seeds are distributed on its edges, like
# the C plotter's pmin/pmax region) so the two always coincide.
PHASE_Z_RANGE = (0.16, 0.44)
PHASE_VZ_RANGE = (-2.2, 2.2)

SIM_DT = 1e-4  # the paper's integration step
RECORD_EVERY = 10  # sample the state at 1 kHz for history/export


@dataclass(frozen=True)
class Params:
    """Commanded controller/model parameters (paper defaults)."""

    za: float = 0.28
    zh: float = 0.26
    zm: float = 0.255
    zb: float = 0.23
    rho: float = 0.0
    k: float = 4.0
    q_scale: float = 1.0
    mass: float = 10.0
    soft_landing: bool = True


def clamp(value: float, lo: float, hi: float) -> float:
    return min(max(value, lo), hi)


def clamp_param(params: Params, name: str, value: float) -> float:
    """Clamp a requested slider value so the kinematic constraints hold.

    Enforces ``zb < zm < zh`` and ``zb < za`` (with an EPS margin) by
    limiting the parameter being moved; the other parameters stay put.
    """
    if name == "za":
        return clamp(value, max(ZA_RANGE[0], params.zb + EPS), ZA_RANGE[1])
    if name == "zm":
        return clamp(value, max(ZM_RANGE[0], params.zb + EPS), min(ZM_RANGE[1], params.zh - EPS))
    if name == "zb":
        hi = min(ZB_RANGE[1], params.zm - EPS, params.za - EPS)
        return clamp(value, ZB_RANGE[0], hi)
    if name == "rho":
        return clamp(value, 0.0, 1.0)
    if name == "k":
        return clamp(value, *K_RANGE)
    if name == "q_scale":
        return clamp(value, *Q_SCALE_RANGE)
    msg = f"unknown parameter: {name}"
    raise ValueError(msg)


def with_param(params: Params, name: str, value: float) -> Params:
    """Return a copy of ``params`` with ``name`` set to the clamped value."""
    return replace(params, **{name: clamp_param(params, name, value)})
