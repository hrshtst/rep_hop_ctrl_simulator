"""Background computation of phase-portrait solution curves.

Reproduces the paper's phase-portrait curve family: initial seeds are
distributed along the edges of the view region — mirroring
``ppp_generate_edge_points()`` in ``src/rhc_phase_portrait_plotter.c``
and its use in ``graph/prog/phase_portrait.c`` — because solution curves
flow inward and cannot cross, so boundary seeds sweep the whole region
without redundant interior curves. When the controller oscillates, a
pair of seeds straddling the orbit center is added to reveal the limit
cycle from inside and outside. When the soft-landing strategy is
enabled, three seeds are pruned exactly as in the paper's figures (the
"remove points for soft landing" block of phase_portrait.c): their
trajectories collapse onto the same adjusted landing path as their
neighbours and would only overdraw the plot.

The rollouts run in the bindings with the GIL released, on a dedicated
worker thread with latest-wins semantics; the UI polls
:meth:`take_result` from its frame timer.
"""

from __future__ import annotations

import math
import threading
from typing import TYPE_CHECKING, NamedTuple

import numpy as np

import rhc
from rhc_demo.params import PHASE_VZ_RANGE, PHASE_Z_RANGE

if TYPE_CHECKING:
    from rhc_demo.params import Params

# Seeds per edge of the phase-portrait view region (the C plotter's
# default n_sc is 10 per axis). The region itself is PHASE_Z_RANGE x
# PHASE_VZ_RANGE, i.e. exactly the view box drawn by the phase view.
N_Z = 10  # along each horizontal edge (stepping z)
N_VZ = 10  # along each vertical edge (stepping vz)
EPSILON = 1e-6

CURVE_DURATION = 0.8  # s of rollout per seed (upper bound; see REGION)
CURVE_DT = 4e-4
CURVE_STRIDE = 4

# Rollouts stop as soon as they leave the view box (the C plotter's
# out-of-region check), so no time is spent evolving off-view segments.
REGION = (*PHASE_Z_RANGE, *PHASE_VZ_RANGE)

Seed = tuple[float, float]
Curve = tuple[np.ndarray, np.ndarray]


class Portrait(NamedTuple):
    """One background-computed phase portrait.

    ``cycle`` is the true limit cycle (stance arc plus flight parabola)
    and ``ellipse`` the full stance ellipse without the lift-off
    cut-off; both are None when the standing equilibrium is stable
    instead (rho <= exp(-k)).
    """

    seeds: list[Seed]
    curves: list[Curve]
    cycle: Curve | None
    ellipse: Curve | None


def _edge_seeds() -> list[Seed]:
    """Return seeds walking the perimeter of the view region.

    Same point set as ``ppp_generate_edge_points()``: each edge is stepped
    from one corner and stops short of the next, so every corner appears
    exactly once.
    """
    z0, z1 = PHASE_Z_RANGE
    v0, v1 = PHASE_VZ_RANGE
    dz = (z1 - z0) / N_Z
    dv = (v1 - v0) / N_VZ
    seeds: list[Seed] = []
    seeds += [(z0 + j * dz, v0) for j in range(N_Z)]  # bottom, left -> right
    seeds += [(z1, v0 + j * dv) for j in range(N_VZ)]  # right, bottom -> top
    seeds += [(z1 - j * dz, v1) for j in range(N_Z)]  # top, right -> left
    seeds += [(z0, v1 - j * dv) for j in range(N_VZ)]  # left, top -> bottom
    return seeds


def _soft_landing_removals(params: Params) -> set[Seed]:
    """Return the seeds pruned when the soft-landing strategy is enabled.

    Mirrors the "remove points for soft landing" block in
    graph/prog/phase_portrait.c. The C code hardcodes three points for
    the paper's region; in grid terms they are the bottom-edge seed
    nearest the lift-off height zh, its left neighbour, and the
    right-edge seed one step below vz = 0, which is how they generalize
    to any region. Points that fall outside the generated seed set are
    ignored, like ppp_remove_p0()'s not-found case.
    """
    z0, z1 = PHASE_Z_RANGE
    v0, v1 = PHASE_VZ_RANGE
    dz = (z1 - z0) / N_Z
    dv = (v1 - v0) / N_VZ
    i_zh = round((params.zh - z0) / dz)
    j_below_zero = math.ceil(-v0 / dv) - 1  # largest grid value < 0
    return {
        (z0 + i_zh * dz, v0),
        (z0 + (i_zh - 1) * dz, v0),
        (z1, v0 + j_below_zero * dv),
    }


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
    """Return edge seeds, minus soft-landing prunes, plus straddle points."""
    removals = _soft_landing_removals(params) if params.soft_landing else set()
    seeds = [s for s in _edge_seeds() if s not in removals]
    if params.rho > 0.0:
        center = _orbit_center(params)
        seeds.append((center - EPSILON, 0.0))
        seeds.append((center + EPSILON, 0.0))
    return seeds


def compute_curves(params: Params) -> Portrait:
    """Roll out the seed family and limit-cycle orbits (blocking).

    Returns the seeds together with their curves so consumers (e.g. the
    phase view's debug seed markers) always show the pair atomically,
    plus the steady-state limit cycle traced by the bindings and its
    full stance ellipse (both None when the standing equilibrium is
    stable and no cycle exists).
    """
    sim = rhc.DynmorphSim(mass=params.mass)
    sim.za = params.za
    sim.zh = params.zh
    sim.zm = params.zm
    sim.zb = params.zb
    sim.rho = params.rho
    sim.k = params.k
    sim.q_scale = params.q_scale
    sim.soft_landing = params.soft_landing
    seeds = default_seeds(params)
    curves = sim.solution_curves(seeds, CURVE_DURATION, CURVE_DT, CURVE_STRIDE, region=REGION)
    cycle_z, cycle_vz = sim.limit_cycle()
    ellipse_z, ellipse_vz = sim.stance_ellipse()
    return Portrait(
        seeds,
        curves,
        (cycle_z, cycle_vz) if len(cycle_z) else None,
        (ellipse_z, ellipse_vz) if len(ellipse_z) else None,
    )


class CurveWorker:
    """Latest-wins background curve computation."""

    def __init__(self) -> None:
        self._cond = threading.Condition()
        self._pending: Params | None = None
        self._result: Portrait | None = None
        self._running = True
        self._thread = threading.Thread(target=self._loop, name="curves", daemon=True)
        self._thread.start()

    def request(self, params: Params) -> None:
        """Queue a recomputation; supersedes any not-yet-started request."""
        with self._cond:
            self._pending = params
            self._cond.notify()

    def take_result(self) -> Portrait | None:
        """Return a freshly computed portrait once, or None if not ready."""
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
