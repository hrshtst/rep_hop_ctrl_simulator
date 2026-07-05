"""Constraint-clamping tests: zb < zm < zh and zb < za must always hold."""

from __future__ import annotations

import pytest

from rhc_demo.params import EPS, Params, clamp_param, with_param


def test_defaults_match_paper():
    p = Params()
    assert (p.za, p.zh, p.zm, p.zb) == (0.28, 0.26, 0.255, 0.23)
    assert p.rho == 0.0
    assert p.k == 4.0
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


def test_unknown_param_raises():
    with pytest.raises(ValueError, match="unknown"):
        clamp_param(Params(), "zz", 0.1)


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
