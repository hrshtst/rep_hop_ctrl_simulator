"""CLI parsing tests (no Qt: the GUI imports are lazy)."""

from __future__ import annotations

from rhc_demo.__main__ import PARAM_OPTIONS, _build_parser


def test_interactive_accepts_initial_parameters():
    args = _build_parser().parse_args(
        ["interactive", "--rho", "1", "--za", "0.32", "--zm", "0.25", "--zb", "0.22", "--k", "8", "--q", "1.5"],
    )
    assert args.rho == 1.0
    assert args.za == 0.32
    assert args.zm == 0.25
    assert args.zb == 0.22
    assert args.k == 8.0
    assert args.q_scale == 1.5  # --q maps onto the q_scale parameter


def test_interactive_parameters_default_to_none():
    args = _build_parser().parse_args(["interactive"])
    assert all(getattr(args, name) is None for name in PARAM_OPTIONS)


def test_bare_invocation_yields_empty_overrides():
    args = _build_parser().parse_args([])
    overrides = {name: getattr(args, name, None) for name in PARAM_OPTIONS}
    assert all(value is None for value in overrides.values())
    assert getattr(args, "paused", False) is False


def test_paused_flag_on_interactive_and_replay():
    parser = _build_parser()
    assert parser.parse_args(["interactive"]).paused is False
    assert parser.parse_args(["interactive", "--paused"]).paused is True
    assert parser.parse_args(["replay", "x.csv", "--paused"]).paused is True
