"""Phase-portrait trajectory family.

Reproduces the paper's phase portrait (a family of (z, vz) solution curves
converging to the equilibrium point for rho=0 or the limit cycle for rho=1) by
rolling out a grid of initial conditions through the bindings, entirely in
memory. Recomputed only when control parameters change, never per frame.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np

import rhc
from rhc_demo.driver import Params, make_system

if TYPE_CHECKING:
    from collections.abc import Sequence

EPSILON = 1e-6

Seed = tuple[float, float]
Trajectory = tuple[np.ndarray, np.ndarray]


def default_seeds(
    params: Params,
    *,
    n_z: int = 10,
    n_vz: int = 10,
    z_range: tuple[float, float] = (0.16, 0.36),
    vz_range: tuple[float, float] = (-1.5, 1.5),
) -> list[Seed]:
    """Return seed initial conditions for the phase-portrait family.

    A grid over the (z, vz) region, plus (when hopping) points straddling the
    equilibrium so the limit cycle is revealed, mirroring prog/phase_portrait.c.
    """
    zs = np.linspace(z_range[0], z_range[1], n_z)
    vzs = np.linspace(vz_range[0], vz_range[1], n_vz)
    seeds: list[Seed] = [(float(z), float(vz)) for z in zs for vz in vzs]
    if params.rho > 0.0:
        seeds.append((params.zm - EPSILON, 0.0))
        seeds.append((params.zm + EPSILON, 0.0))
    return seeds


def build_field(
    params: Params,
    seeds: Sequence[Seed] | None = None,
    duration: float = 1.0,
    dt: float = 1e-4,
) -> list[Trajectory]:
    """Roll out each seed and return its (z, vz) arrays.

    A dedicated system is built so the caller's live simulation is untouched;
    rollout() resets the simulator between seeds.
    """
    if seeds is None:
        seeds = default_seeds(params)
    system = make_system(params)
    trajectories: list[Trajectory] = []
    for z0, vz0 in seeds:
        out = system.sim.rollout(rhc.Vec([z0, vz0]), duration, dt)
        trajectories.append((out["z"], out["vz"]))
    return trajectories
