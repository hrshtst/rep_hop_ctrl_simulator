"""Solution-curve seed selection and worker round-trip tests."""

from __future__ import annotations

import time

import numpy as np
import pytest

from rhc_demo.curves import (
    N_VZ,
    N_Z,
    CurveWorker,
    compute_curves,
    default_seeds,
)
from rhc_demo.params import PHASE_VZ_RANGE, PHASE_Z_RANGE, Params, with_param

N_EDGE = 2 * (N_Z + N_VZ)  # seeds on the region perimeter


def test_seeds_lie_on_region_edges_when_standing():
    seeds = default_seeds(Params())  # rho = 0: edge points only
    assert len(seeds) == N_EDGE
    for z, vz in seeds:
        on_horizontal = vz in PHASE_VZ_RANGE and PHASE_Z_RANGE[0] <= z <= PHASE_Z_RANGE[1]
        on_vertical = z in PHASE_Z_RANGE and PHASE_VZ_RANGE[0] <= vz <= PHASE_VZ_RANGE[1]
        assert on_horizontal or on_vertical, f"seed ({z}, {vz}) not on an edge"


def test_each_corner_appears_exactly_once():
    seeds = default_seeds(Params())
    assert len(set(seeds)) == len(seeds)
    for corner_z in PHASE_Z_RANGE:
        for corner_vz in PHASE_VZ_RANGE:
            assert seeds.count((corner_z, corner_vz)) == 1


def test_straddle_points_bracket_zm_when_hopping():
    p = Params(rho=1.0)  # za > zh: hop
    seeds = default_seeds(p)
    assert len(seeds) == N_EDGE + 2
    (z1, v1), (z2, v2) = seeds[-2:]
    assert v1 == v2 == 0.0
    assert z1 < p.zm < z2


def test_straddle_points_bracket_squat_center():
    p = with_param(Params(rho=1.0), "za", 0.25)  # za < zh: squat
    seeds = default_seeds(p)
    center = 0.5 * (p.za + p.zb)
    (z1, _), (z2, _) = seeds[-2:]
    assert z1 < center < z2


def test_compute_curves_pairs_each_seed_with_its_curve():
    seeds, curves = compute_curves(Params(rho=1.0))
    assert len(seeds) == len(curves)
    for (z0, vz0), (z, vz) in zip(seeds, curves, strict=True):
        assert z[0] == pytest.approx(z0)
        assert vz[0] == pytest.approx(vz0)


def test_worker_returns_result_once():
    worker = CurveWorker()
    try:
        worker.request(Params())
        deadline = time.monotonic() + 5.0
        result = None
        while result is None and time.monotonic() < deadline:
            result = worker.take_result()
            time.sleep(0.01)
        assert result is not None
        seeds, curves = result
        assert len(seeds) == len(curves) == N_EDGE
        assert worker.take_result() is None
    finally:
        worker.close()


def test_curve_points_stay_inside_view_region():
    _, curves = compute_curves(Params(rho=1.0))
    for z, vz in curves:
        assert np.all((z >= PHASE_Z_RANGE[0]) & (z <= PHASE_Z_RANGE[1]))
        assert np.all((vz >= PHASE_VZ_RANGE[0]) & (vz <= PHASE_VZ_RANGE[1]))
