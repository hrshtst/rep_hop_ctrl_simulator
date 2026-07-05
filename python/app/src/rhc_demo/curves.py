"""Background computation of phase-portrait solution curves.

Reproduces the paper's phase-portrait curve family (see
``graph/make_phase_portraits.sh`` and its concise variant) with a
deliberately reduced seed count so recomputation keeps up with live
slider changes. The rollouts run in the bindings with the GIL released,
on a dedicated worker thread with latest-wins semantics; the UI polls
:meth:`take_result` from its frame timer.
"""

from __future__ import annotations

import threading
from typing import TYPE_CHECKING

import numpy as np

import rhc

if TYPE_CHECKING:
    from rhc_demo.params import Params

# Seed grid over the phase-portrait view region (kept small on purpose).
N_Z = 7
N_VZ = 5
Z_SEED_RANGE = (0.18, 0.38)
VZ_SEED_RANGE = (-1.6, 1.6)
EPSILON = 1e-6

CURVE_DURATION = 0.8  # s of rollout per seed
CURVE_DT = 4e-4
CURVE_STRIDE = 4

Curve = tuple[np.ndarray, np.ndarray]


def default_seeds(params: Params) -> list[tuple[float, float]]:
    """Return a coarse seed grid, plus equilibrium-straddling points when hopping."""
    zs = np.linspace(*Z_SEED_RANGE, N_Z)
    vzs = np.linspace(*VZ_SEED_RANGE, N_VZ)
    seeds = [(float(z), float(vz)) for z in zs for vz in vzs]
    if params.rho > 0.0:
        seeds.append((params.zm - EPSILON, 0.0))
        seeds.append((params.zm + EPSILON, 0.0))
    return seeds


def compute_curves(params: Params) -> list[Curve]:
    """Roll out the seed family with the given parameters (blocking)."""
    sim = rhc.DynmorphSim(mass=params.mass)
    sim.za = params.za
    sim.zh = params.zh
    sim.zm = params.zm
    sim.zb = params.zb
    sim.rho = params.rho
    sim.k = params.k
    sim.soft_landing = params.soft_landing
    return sim.solution_curves(default_seeds(params), CURVE_DURATION, CURVE_DT, CURVE_STRIDE)


class CurveWorker:
    """Latest-wins background curve computation."""

    def __init__(self) -> None:
        self._cond = threading.Condition()
        self._pending: Params | None = None
        self._result: list[Curve] | None = None
        self._running = True
        self._thread = threading.Thread(target=self._loop, name="curves", daemon=True)
        self._thread.start()

    def request(self, params: Params) -> None:
        """Queue a recomputation; supersedes any not-yet-started request."""
        with self._cond:
            self._pending = params
            self._cond.notify()

    def take_result(self) -> list[Curve] | None:
        """Return freshly computed curves once, or None if not ready."""
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
            curves = compute_curves(params)
            with self._cond:
                self._result = curves
