"""Solution-curve seed selection and worker round-trip tests."""

from __future__ import annotations

import time

import pytest

from rhc_demo.curves import N_VZ, N_Z, CurveWorker, compute_curves, default_seeds
from rhc_demo.params import Params

GRID = N_Z * N_VZ


def test_default_seed_grid_when_standing():
    seeds = default_seeds(Params())  # rho = 0
    assert len(seeds) == GRID  # plain grid, no straddle points


def test_straddle_points_added_when_hopping():
    p = Params(rho=1.0)
    seeds = default_seeds(p)
    assert len(seeds) == GRID + 2
    (z1, v1), (z2, v2) = seeds[-2:]
    assert v1 == v2 == 0.0
    assert z1 < p.zm < z2


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
        assert len(seeds) == len(curves) == GRID
        assert worker.take_result() is None
    finally:
        worker.close()
