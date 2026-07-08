"""Control parameters, paper defaults, and kinematic-constraint clamping.

The slider constraints from the directive are enforced here as pure
functions so they can be unit-tested without Qt:
``zb < zm < zh`` and ``zb < za`` must never be violated.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, replace
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Mapping

# Minimum separation kept between constrained heights, in metres.
EPS = 1e-3

# Absolute slider ranges, in metres. zh is a robot constant (not a slider).
ZA_RANGE = (0.20, 0.43)
ZM_RANGE = (0.20, 0.30)
ZB_RANGE = (0.15, 0.30)
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


# CLI-overridable parameters and their absolute slider ranges.
PARAM_RANGES: dict[str, tuple[float, float]] = {
    "rho": (0.0, 1.0),
    "za": ZA_RANGE,
    "zm": ZM_RANGE,
    "zb": ZB_RANGE,
    "k": K_RANGE,
    "q_scale": Q_SCALE_RANGE,
}


def initial_params(overrides: Mapping[str, float | None]) -> tuple[Params, list[str]]:
    """Build the starting :class:`Params` from CLI overrides, sanitized.

    Values outside their slider range are clipped to it. Afterwards the
    kinematic constraints ``zb < zm < zh`` and ``zb < za`` are enforced
    by re-clamping the heights in the order zm, zb, za: zm yields to
    the fixed zh, zb yields to the surrounding heights, and za yields
    to the final zb — one pass restores every constraint. Each change
    is reported in the returned list of human-readable warnings.
    """
    params = Params()
    warnings: list[str] = []
    updates: dict[str, float] = {}
    for name, value in overrides.items():
        if name not in PARAM_RANGES:
            msg = f"unknown parameter: {name}"
            raise ValueError(msg)
        if value is None:
            continue
        if math.isnan(value):
            warnings.append(f"{name} = nan is not a number; using the default {getattr(params, name):g}")
            continue
        lo, hi = PARAM_RANGES[name]
        clipped = clamp(value, lo, hi)
        if clipped != value:
            warnings.append(f"{name} = {value:g} is outside its slider range [{lo:g}, {hi:g}]; clipped to {clipped:g}")
        updates[name] = clipped
    params = replace(params, **updates)
    for name in ("zm", "zb", "za"):
        current = getattr(params, name)
        adjusted = clamp_param(params, name, current)
        if adjusted != current:
            warnings.append(
                f"{name} = {current:g} breaks the kinematic constraints"
                f" (zb < zm < zh, zb < za); adjusted to {adjusted:g}",
            )
            params = replace(params, **{name: adjusted})
    return params, warnings
