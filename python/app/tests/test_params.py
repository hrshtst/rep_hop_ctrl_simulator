"""Constraint-clamping tests: zb < zm < zh and zb < za must always hold."""

from __future__ import annotations

import pytest

from rhc_demo.params import (
    EPS,
    Q_SCALE_RANGE,
    ZA_RANGE,
    ZB_RANGE,
    Params,
    clamp_param,
    initial_params,
    with_param,
)


def test_defaults_match_paper():
    p = Params()
    assert (p.za, p.zh, p.zm, p.zb) == (0.28, 0.26, 0.255, 0.23)
    assert p.rho == 0.0
    assert p.k == 4.0
    assert p.q_scale == 1.0
    assert p.mass == 10.0
    assert p.soft_landing is True


def test_zm_cannot_reach_zh():
    p = Params()
    assert clamp_param(p, "zm", 0.30) == pytest.approx(p.zh - EPS)


def test_zm_cannot_reach_zb():
    p = Params()
    assert clamp_param(p, "zm", 0.20) == pytest.approx(p.zb + EPS)


def test_zb_cannot_reach_zm_or_za():
    p = Params()
    assert clamp_param(p, "zb", 0.29) == pytest.approx(p.zm - EPS)
    p2 = with_param(p, "za", 0.24)  # apex below zm
    assert clamp_param(p2, "zb", 0.29) == pytest.approx(p2.za - EPS)


def test_za_cannot_reach_zb():
    p = with_param(Params(), "zb", 0.25)
    assert clamp_param(p, "za", 0.20) == pytest.approx(p.zb + EPS)


def test_za_may_go_below_zh_for_squatting():
    p = Params()
    assert clamp_param(p, "za", 0.25) == pytest.approx(0.25)


def test_rho_clamped_to_unit_interval():
    p = Params()
    assert clamp_param(p, "rho", 1.5) == 1.0
    assert clamp_param(p, "rho", -0.5) == 0.0


def test_q_scale_clamped_to_range():
    p = Params()
    assert clamp_param(p, "q_scale", 10.0) == Q_SCALE_RANGE[1]
    assert clamp_param(p, "q_scale", 0.0) == Q_SCALE_RANGE[0]
    assert clamp_param(p, "q_scale", 1.3) == pytest.approx(1.3)


def test_unknown_param_raises():
    with pytest.raises(ValueError, match="unknown"):
        clamp_param(Params(), "zz", 0.1)


def test_initial_params_defaults_without_overrides():
    params, warnings = initial_params({})
    assert params == Params()
    assert warnings == []
    params, warnings = initial_params({"rho": None, "za": None})
    assert params == Params()
    assert warnings == []


def test_initial_params_accepts_in_range_values_silently():
    params, warnings = initial_params({"rho": 1.0, "za": 0.32, "k": 8.0, "q_scale": 1.5})
    assert warnings == []
    assert (params.rho, params.za, params.k, params.q_scale) == (1.0, 0.32, 8.0, 1.5)


def test_initial_params_clips_to_slider_ranges_with_warning():
    params, warnings = initial_params({"za": 0.50, "rho": -0.2})
    assert params.za == ZA_RANGE[1]
    assert params.rho == 0.0
    assert len(warnings) == 2
    assert all("clipped" in w for w in warnings)


def test_initial_params_enforces_kinematic_constraints_with_warning():
    # zb above zm: zb yields to the surrounding heights.
    params, warnings = initial_params({"zm": 0.22, "zb": 0.24})
    assert params.zb < params.zm < params.zh
    assert params.zb < params.za
    assert params.zb == pytest.approx(0.24)  # user's zb honored
    assert params.zm == pytest.approx(0.24 + EPS)  # zm moved above it
    assert any("kinematic" in w for w in warnings)


def test_initial_params_clips_then_adjusts():
    # 0.31 is out of range (clipped to 0.30) and then infeasible under
    # zb < zm < zh: zm rises to its ceiling to give zb maximal room,
    # and zb settles just below it.
    params, warnings = initial_params({"zb": 0.31})
    assert params.zm == pytest.approx(params.zh - EPS)
    assert params.zb == pytest.approx(params.zh - 2 * EPS)
    assert len(warnings) == 3
    assert "clipped" in warnings[0]
    assert "kinematic" in warnings[1]
    assert "kinematic" in warnings[2]


def test_initial_params_ignores_nan_with_warning():
    params, warnings = initial_params({"k": float("nan")})
    assert params.k == Params().k
    assert len(warnings) == 1


def test_initial_params_rejects_unknown_names():
    with pytest.raises(ValueError, match="unknown"):
        initial_params({"zh": 0.3})


def test_initial_params_invariants_hold_for_adversarial_combos():
    cases = [
        {"za": 0.21},
        {"zb": 0.29},
        {"zm": 0.30, "zb": 0.30},
        {"za": 0.20, "zm": 0.30, "zb": 0.30},
        {"za": 1.0, "zm": 0.0, "zb": 1.0},
        {"za": ZB_RANGE[0], "zb": ZB_RANGE[0]},
    ]
    for overrides in cases:
        params, _ = initial_params(overrides)
        assert params.zb < params.zm < params.zh, overrides
        assert params.zb < params.za, overrides


def test_constraint_invariants_hold_after_any_single_move():
    p = Params()
    for name, value in [
        ("zm", 0.35),
        ("zm", 0.10),
        ("zb", 0.35),
        ("zb", 0.10),
        ("za", 0.10),
        ("za", 0.45),
        ("rho", 2.0),
        ("k", 100.0),
    ]:
        q = with_param(p, name, value)
        assert q.zb < q.zm < q.zh
        assert q.zb < q.za
