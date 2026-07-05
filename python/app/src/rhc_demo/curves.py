"""Background computation of phase-portrait solution curves.

Reproduces the paper's phase-portrait curve family: initial seeds are
distributed along the edges of the view region — mirroring
``ppp_generate_edge_points()`` in ``src/rhc_phase_portrait_plotter.c``
and its use in ``graph/prog/phase_portrait.c`` — because solution curves
flow inward and cannot cross, so boundary seeds sweep the whole region
without redundant interior curves. When the controller oscillates, a
pair of seeds straddling the orbit center is added to reveal the limit
cycle from inside and outside.

The rollouts run in the bindings with the GIL released, on a dedicated
worker thread with latest-wins semantics; the UI polls
:meth:`take_result` from its frame timer.
"""

from __future__ import annotations

import threading
from typing import TYPE_CHECKING

import numpy as np

import rhc

if TYPE_CHECKING:
    from rhc_demo.params import Params

# Seeds per edge of the phase-portrait view region (the C plotter's
# default n_sc is 10 per axis).
N_Z = 10  # along each horizontal edge (stepping z)
N_VZ = 10  # along each vertical edge (stepping vz)
Z_SEED_RANGE = (0.18, 0.38)
VZ_SEED_RANGE = (-1.6, 1.6)
EPSILON = 1e-6

CURVE_DURATION = 0.8  # s of rollout per seed
CURVE_DT = 4e-4
CURVE_STRIDE = 4

Seed = tuple[float, float]
Curve = tuple[np.ndarray, np.ndarray]


def _edge_seeds() -> list[Seed]:
    """Return seeds walking the perimeter of the view region.

    Same point set as ``ppp_generate_edge_points()``: each edge is stepped
    from one corner and stops short of the next, so every corner appears
    exactly once.
    """
    z0, z1 = Z_SEED_RANGE
    v0, v1 = VZ_SEED_RANGE
    dz = (z1 - z0) / N_Z
    dv = (v1 - v0) / N_VZ
    seeds: list[Seed] = []
    seeds += [(z0 + j * dz, v0) for j in range(N_Z)]  # bottom, left -> right
    seeds += [(z1, v0 + j * dv) for j in range(N_VZ)]  # right, bottom -> top
    seeds += [(z1 - j * dz, v1) for j in range(N_Z)]  # top, right -> left
    seeds += [(z0, v1 - j * dv) for j in range(N_VZ)]  # left, top -> bottom
    return seeds


def _orbit_center(params: Params) -> float:
    """Return the stance-orbit center the straddle seeds should bracket.

    Mirrors graph/prog/phase_portrait.c: the squat center (za + zb)/2 when
    the target apex is below the lift-off height, the standing height zm
    otherwise.
    """
    if params.za < params.zh:
        return 0.5 * (params.za + params.zb)
    return params.zm


def default_seeds(params: Params) -> list[Seed]:
    """Return edge seeds, plus orbit-straddling points when oscillating."""
    seeds = _edge_seeds()
    if params.rho > 0.0:
        center = _orbit_center(params)
        seeds.append((center - EPSILON, 0.0))
        seeds.append((center + EPSILON, 0.0))
    return seeds


def compute_curves(params: Params) -> tuple[list[Seed], list[Curve]]:
    """Roll out the seed family with the given parameters (blocking).

    Returns the seeds together with their curves so consumers (e.g. the
    phase view's debug seed markers) always show the pair atomically.
    """
    sim = rhc.DynmorphSim(mass=params.mass)
    sim.za = params.za
    sim.zh = params.zh
    sim.zm = params.zm
    sim.zb = params.zb
    sim.rho = params.rho
    sim.k = params.k
    sim.soft_landing = params.soft_landing
    seeds = default_seeds(params)
    return seeds, sim.solution_curves(seeds, CURVE_DURATION, CURVE_DT, CURVE_STRIDE)


class CurveWorker:
    """Latest-wins background curve computation."""

    def __init__(self) -> None:
        self._cond = threading.Condition()
        self._pending: Params | None = None
        self._result: tuple[list[Seed], list[Curve]] | None = None
        self._running = True
        self._thread = threading.Thread(target=self._loop, name="curves", daemon=True)
        self._thread.start()

    def request(self, params: Params) -> None:
        """Queue a recomputation; supersedes any not-yet-started request."""
        with self._cond:
            self._pending = params
            self._cond.notify()

    def take_result(self) -> tuple[list[Seed], list[Curve]] | None:
        """Return freshly computed (seeds, curves) once, or None if not ready."""
        with self._cond:
            result = self._result
            self._result = None
            return result

    def close(self) -> None:
        with self._cond:
            self._running = False
            self._cond.notify()
        self._thread.join(timeout=2.0)

    def _loop(self) -> None:
        while True:
            with self._cond:
                while self._running and self._pending is None:
                    self._cond.wait()
                if not self._running:
                    return
                params = self._pending
                self._pending = None
            result = compute_curves(params)
            with self._cond:
                self._result = result
