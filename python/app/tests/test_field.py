from __future__ import annotations

import numpy as np

from rhc_demo.driver import Params
from rhc_demo.field import build_field, default_seeds


def test_default_seeds_grid_size():
    seeds = default_seeds(Params(rho=0.0), n_z=10, n_vz=10)
    assert len(seeds) == 100  # rho=0: no extra equilibrium straddle seeds


def test_default_seeds_adds_equilibrium_for_hopping():
    seeds = default_seeds(Params(rho=1.0), n_z=10, n_vz=10)
    assert len(seeds) > 100


def test_build_field_returns_one_trajectory_per_seed():
    params = Params(rho=1.0)
    seeds = default_seeds(params, n_z=4, n_vz=4)
    field = build_field(params, seeds=seeds, duration=0.3, dt=1e-3)
    assert len(field) == len(seeds)
    for z, vz in field:
        assert isinstance(z, np.ndarray)
        assert isinstance(vz, np.ndarray)
        assert len(z) == len(vz)
        assert len(z) > 0
