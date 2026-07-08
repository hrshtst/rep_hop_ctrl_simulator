"""Tests for the DynmorphSim facade against the paper's expected behavior."""

from __future__ import annotations

import numpy as np
import pytest

import rhc

DT = 1e-4


def test_defaults_match_paper():
    sim = rhc.DynmorphSim()
    assert sim.za == pytest.approx(0.28)
    assert sim.zh == pytest.approx(0.26)
    assert sim.zm == pytest.approx(0.255)
    assert sim.zb == pytest.approx(0.23)
    assert sim.rho == 0.0
    assert sim.k == pytest.approx(4.0)
    assert sim.q_scale == pytest.approx(1.0)
    assert sim.soft_landing is True
    assert sim.mass == pytest.approx(10.0)
    assert sim.gravity == pytest.approx(9.80665, abs=1e-4)


def test_initial_state_is_paper_stand_start():
    sim = rhc.DynmorphSim()
    assert sim.stand_start_z == pytest.approx(0.2575)
    assert sim.z == pytest.approx(0.2575)
    assert sim.vz == 0.0


def test_standing_settles_to_zm():
    sim = rhc.DynmorphSim()
    sim.advance(30_000, DT)  # 3 s
    assert sim.z == pytest.approx(sim.zm, abs=1e-4)
    assert sim.vz == pytest.approx(0.0, abs=1e-3)


def test_hopping_apex_converges_to_za():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    out = sim.advance(50_000, DT)
    assert out["ap_z"][-1] == pytest.approx(0.28, abs=1e-3)
    assert sim.hops > 10


def test_advance_records_are_continuous_across_calls():
    sim = rhc.DynmorphSim()
    a = sim.advance(100, DT)
    b = sim.advance(100, DT)
    t = np.concatenate([a["t"], b["t"]])
    assert np.all(np.diff(t) > 0)
    assert b["t"][0] == pytest.approx(a["t"][-1] + DT)


def test_record_every_decimates():
    sim = rhc.DynmorphSim()
    out = sim.advance(1000, DT, record_every=10)
    assert len(out["t"]) == 100
    assert np.allclose(np.diff(out["t"]), 10 * DT)


def test_records_cover_csv_header():
    sim = rhc.DynmorphSim()
    out = sim.advance(10, DT)
    names = sim.csv_header().split(",")
    assert names[0] == "tag"
    for name in names[1:]:
        assert name in out, f"column {name} missing from records"
    assert len(names) == 41


def test_rollout_does_not_disturb_live_state():
    sim = rhc.DynmorphSim()
    sim.advance(1000, DT)
    t0, z0 = sim.t, sim.z
    out = sim.rollout(0.30, 0.0, 0.5, DT)
    assert len(out["t"]) == 5000
    assert out["z"][0] == pytest.approx(0.30)
    assert sim.t == t0
    assert sim.z == z0


def test_solution_curves_shapes():
    sim = rhc.DynmorphSim()
    curves = sim.solution_curves([(0.24, 0.0), (0.30, 0.5), (0.20, -1.0)], duration=0.2, dt=4e-4, stride=5)
    assert len(curves) == 3
    for z, vz in curves:
        assert len(z) == len(vz) == 100
    assert curves[0][0][0] == pytest.approx(0.24)
    assert curves[1][1][0] == pytest.approx(0.5)


def test_reset_restores_stand_start_and_clears_events():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    sim.advance(30_000, DT)
    assert sim.hops > 0
    sim.reset()
    assert sim.t == 0.0
    assert sim.z == pytest.approx(sim.stand_start_z)
    assert sim.hops == 0
    assert sim.fe == 0.0


def test_external_force_is_scalar_and_applied():
    sim = rhc.DynmorphSim()
    sim.advance(20_000, DT)  # settle at standing
    z_settled = sim.z
    sim.fe = 200.0  # push up with ~2x weight
    sim.advance(3000, DT)
    assert sim.z > z_settled + 0.005
    sim.fe = 0.0


def test_squatting_stays_below_zh():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    sim.za = 0.255  # squatting: apex target below lift-off height
    sim.advance(50_000, DT)
    out = sim.advance(20_000, DT)
    assert np.max(out["z"]) < sim.zh + 1e-6


def test_soft_landing_flag_is_recorded():
    sim = rhc.DynmorphSim()
    sim.soft_landing = False
    out = sim.advance(10, DT)
    assert np.all(out["soft_landing"] == 0)
    sim.soft_landing = True
    out = sim.advance(10, DT)
    assert np.all(out["soft_landing"] == 1)


def test_invalid_step_args_raise():
    sim = rhc.DynmorphSim()
    with pytest.raises(ValueError, match="dt"):
        sim.advance(10, -1.0)
    with pytest.raises(ValueError, match="step count"):
        sim.advance(-5, DT)
    with pytest.raises(ValueError, match="record_every"):
        sim.advance(10, DT, 0)


def test_phase_enum_values():
    assert int(rhc.Phase.INVALID) == -1
    assert int(rhc.Phase.FALLING) == 0
    assert int(rhc.Phase.COMPRESSION) == 1
    assert int(rhc.Phase.EXTENSION) == 2
    assert int(rhc.Phase.RISING) == 3


def test_solution_curves_stop_when_leaving_region():
    sim = rhc.DynmorphSim()
    region = (0.16, 0.40, -2.0, 2.0)
    seed = [(0.39, 1.9)]  # near the top-right corner, moving up and out
    n_full = int(0.5 / 4e-4)
    (full,) = sim.solution_curves(seed, duration=0.5, dt=4e-4, stride=1)
    (bounded,) = sim.solution_curves(seed, duration=0.5, dt=4e-4, stride=1, region=region)
    assert len(full[0]) == n_full
    assert len(bounded[0]) < n_full / 10  # exits the box almost immediately
    z, vz = bounded
    assert np.all((z >= region[0]) & (z <= region[1]))
    assert np.all((vz >= region[2]) & (vz <= region[3]))


def test_solution_curves_on_boundary_seed_counts_as_inside():
    sim = rhc.DynmorphSim()
    region = (0.16, 0.40, -2.0, 2.0)
    (curve,) = sim.solution_curves([(0.40, 2.0)], duration=0.1, dt=4e-4, stride=1, region=region)
    assert len(curve[0]) >= 1  # the corner seed itself is recorded


def test_limit_cycle_absent_for_regulator():
    sim = rhc.DynmorphSim()  # rho = 0
    z, vz = sim.limit_cycle()
    assert len(z) == len(vz) == 0


def test_limit_cycle_absent_below_bifurcation():
    sim = rhc.DynmorphSim()
    sim.rho = 0.9 * float(np.exp(-sim.k))  # just below rho = exp(-k)
    z, _vz = sim.limit_cycle()
    assert len(z) == 0


def test_limit_cycle_matches_designed_hopping_orbit():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    z, vz = sim.limit_cycle()
    assert len(z) == len(vz) > 10
    # Closed loop, apex at the target and bottom at the adjusted zb
    # (calc_zb(0.28, 0.26, 0.255) = 0.24 in the paper's nominal design).
    assert z[0] == z[-1]
    assert vz[0] == vz[-1]
    assert z.max() == pytest.approx(sim.za, abs=1e-3)
    assert z.min() == pytest.approx(0.24, abs=1e-3)
    assert vz.max() == pytest.approx(-vz.min(), abs=1e-2)


def test_limit_cycle_shrinks_with_rho():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    z_full, _ = sim.limit_cycle()
    sim.rho = 0.5
    z_half, _ = sim.limit_cycle()
    assert 0.0 < z_half.max() - z_half.min() < z_full.max() - z_full.min()


def test_limit_cycle_squat_stays_below_zh():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    sim.za = 0.255  # below zh: continuous squatting
    z, _vz = sim.limit_cycle()
    assert len(z) > 10
    assert z.max() <= sim.zh + 1e-6
    assert z.min() == pytest.approx(sim.zb, abs=1e-3)


def test_stance_ellipse_ignores_liftoff_cutoff():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    z, vz = sim.stance_ellipse()
    assert len(z) == len(vz) == 181
    assert z[0] == pytest.approx(z[-1])  # closed loop
    # Nominal design: zb adjusted to 0.24, so the full ellipse spans
    # zm +/- r = 0.255 +/- 0.015 — its top exceeds zh = 0.26.
    assert z.max() == pytest.approx(0.27, abs=1e-6)
    assert z.min() == pytest.approx(0.24, abs=1e-6)


def test_stance_ellipse_empty_when_standing():
    sim = rhc.DynmorphSim()  # rho = 0
    z, vz = sim.stance_ellipse()
    assert len(z) == len(vz) == 0


def test_module_stance_ellipse_matches_soft_landing_orbit():
    # Boosted apex 0.35: the cushion orbit keeps zb = 0.23 and raises the
    # center to zm' = (za' + zb - (za'-zh)^2/(za'-zb)) / 2 (paper Eq. 16).
    zm_prime = 0.5 * (0.35 + 0.23 - (0.35 - 0.26) ** 2 / (0.35 - 0.23))
    z, vz = rhc.stance_ellipse(zh=0.26, zm=zm_prime, zb=0.23, rho=1.0, k=4.0)
    assert z.min() == pytest.approx(0.23, abs=1e-6)
    assert z.max() == pytest.approx(2 * zm_prime - 0.23, abs=1e-6)
    assert vz.max() == pytest.approx(-vz.min(), abs=1e-9)
    zero, _ = rhc.stance_ellipse(zh=0.26, zm=0.255, zb=0.23, rho=0.01)
    assert len(zero) == 0


def test_limit_cycle_leaves_live_state_untouched():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    before = (sim.t, sim.z, sim.vz)
    sim.limit_cycle()
    assert (sim.t, sim.z, sim.vz) == before
