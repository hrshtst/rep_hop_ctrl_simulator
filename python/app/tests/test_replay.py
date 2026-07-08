"""Replay-source tests: cursor pacing, stepping, and snapshot fidelity."""

from __future__ import annotations

import numpy as np
import pytest

import rhc
from rhc_demo.csv_io import concat_records
from rhc_demo.replay import ReplaySource


@pytest.fixture
def data():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    return concat_records([sim.advance(10_000, 1e-4, 10)])  # 1 s at 1 kHz


def test_duration(data):
    src = ReplaySource(data)
    assert src.duration == pytest.approx(1.0, abs=0.01)


def test_seek_produces_matching_snapshot(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(0.5)
    snap, _ = src.frame()
    i = int(np.searchsorted(data["t"], 0.5, side="right") - 1)
    assert snap.t == pytest.approx(data["t"][i])
    assert snap.z == pytest.approx(data["z"][i])
    assert snap.phase == data["phase"][i]
    assert snap.rho == pytest.approx(data["rho"][i])


def test_frame_emits_each_sample_once(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(0.3)
    _, chunks1 = src.frame()
    n1 = sum(len(z) for z, _ in chunks1)
    _, chunks2 = src.frame()
    n2 = sum(len(z) for z, _ in chunks2)
    assert n1 > 0
    assert n2 == 0


def test_step_advances_cursor_while_paused(data):
    src = ReplaySource(data)
    src.set_paused(True)
    snap0, _ = src.frame()
    src.request_step(0.05)
    snap1, _ = src.frame()
    assert snap1.t - snap0.t == pytest.approx(0.05, abs=0.005)


def test_step_back_rewinds_cursor_while_paused(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(0.5)
    snap0, _ = src.frame()
    src.request_step_back(0.05)
    snap1, _ = src.frame()
    assert snap0.t - snap1.t == pytest.approx(0.05, abs=0.005)
    # Rewinding never emits duplicate history when re-advancing.
    src.request_step(0.05)
    _, chunks = src.frame()
    assert sum(len(z) for z, _ in chunks) == 0


def test_step_back_clamps_at_start(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(0.02)
    src.request_step_back(1.0)
    snap, _ = src.frame()
    assert snap.t == pytest.approx(data["t"][0])


def test_reset_rewinds(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(0.8)
    src.frame()
    src.reset()
    src.set_paused(True)
    snap, _ = src.frame()
    assert snap.t == pytest.approx(data["t"][0])


def test_finishes_at_end(data):
    src = ReplaySource(data)
    src.set_paused(True)
    src.seek_time(10.0)  # beyond the end: clamps
    snap, _ = src.frame()
    assert snap.finished
    assert snap.t == pytest.approx(data["t"][-1])


def test_missing_optional_columns_default(data):
    minimal = {k: data[k] for k in ("t", "z", "vz")}
    src = ReplaySource(minimal)
    src.set_paused(True)
    snap, _ = src.frame()
    assert snap.fz == 0.0
    assert np.isnan(snap.za)
