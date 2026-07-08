"""Live-engine tests: threading, pacing, commands, and export (no Qt needed)."""

from __future__ import annotations

import time

import pytest

from rhc_demo.engine import LiveEngine
from rhc_demo.params import Params


@pytest.fixture
def engine():
    eng = LiveEngine()
    yield eng
    eng.close()


def _wait_until(predicate, timeout=3.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return True
        time.sleep(0.01)
    return False


def test_engine_advances_in_real_time(engine):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.2)
    snap, _ = engine.frame()
    assert snap.playing
    assert snap.z == pytest.approx(0.255, abs=0.01)  # settling toward zm


def test_frame_returns_new_samples_once(engine):
    engine.start()
    assert _wait_until(lambda: bool(engine.frame()[1]))
    time.sleep(0.1)
    _, chunks = engine.frame()
    n = sum(len(z) for z, _ in chunks)
    assert n > 0
    _, again = engine.frame()
    n_again = sum(len(z) for z, _ in again)
    assert n_again < n  # buffers were drained


def test_pause_and_step(engine):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.05)
    engine.set_paused(True)
    assert _wait_until(lambda: not engine.frame()[0].playing)
    t_paused = engine.frame()[0].t
    time.sleep(0.15)
    assert engine.frame()[0].t == pytest.approx(t_paused, abs=1e-9)
    engine.request_step(0.01)
    assert _wait_until(lambda: engine.frame()[0].t >= t_paused + 0.01 - 1e-9)
    assert engine.frame()[0].t == pytest.approx(t_paused + 0.01, abs=1e-3)


def test_param_change_reaches_simulation(engine):
    engine.start()
    engine.set_param("za", 0.30)
    assert _wait_until(lambda: engine.frame()[0].za == pytest.approx(0.30))


def test_rho_is_slewed_not_stepped(engine):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.05)
    engine.set_param("rho", 1.0)
    time.sleep(0.15)
    snap, _ = engine.frame()
    assert snap.rho == 1.0  # command registered immediately
    assert 0.0 < snap.p_rho < 1.0  # applied value still morphing (2/s slew)


def test_reset_returns_to_stand_start(engine):
    engine.start()
    engine.set_param("rho", 1.0)
    assert _wait_until(lambda: engine.frame()[0].t > 0.5)
    engine.reset()
    assert _wait_until(lambda: engine.frame()[0].t < 0.5)
    snap, _ = engine.frame()
    assert snap.z == pytest.approx(0.2575, abs=0.05)


def test_external_force_command(engine):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.1)
    engine.set_fe(300.0)
    assert _wait_until(lambda: engine.frame()[0].fe == pytest.approx(300.0))
    engine.set_fe(0.0)
    assert _wait_until(lambda: engine.frame()[0].fe == 0.0)


def test_export_csv(engine, tmp_path):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.3)
    engine.set_paused(True)
    path = tmp_path / "out.csv"
    rows = engine.export_csv(path)
    assert rows > 0
    header = path.read_text().splitlines()[0]
    assert header.startswith("tag,t,z,vz,")
    from rhc_demo.csv_io import read_timeseries

    data = read_timeseries(path)
    assert len(data["t"]) == rows


def test_custom_params_applied():
    eng = LiveEngine(Params(za=0.31, rho=1.0, q_scale=1.5, soft_landing=False))
    try:
        snap, _ = eng.frame()
        assert snap.za == pytest.approx(0.31)
        assert snap.q_scale == pytest.approx(1.5)
        assert snap.soft_landing is False
    finally:
        eng.close()


def test_playback_speed_slows_simulated_time(engine):
    engine.start()
    engine.set_speed(0.25)
    assert _wait_until(lambda: engine.frame()[0].t > 0.01)
    t0 = engine.frame()[0].t
    time.sleep(0.4)
    elapsed = engine.frame()[0].t - t0
    assert elapsed == pytest.approx(0.1, abs=0.05)  # quarter of wall time


def test_reset_frees_recorded_export_history(engine, tmp_path):
    engine.start()
    assert _wait_until(lambda: engine.frame()[0].t > 0.3)
    engine.reset()
    assert _wait_until(lambda: engine.frame()[0].t < 0.3)
    engine.set_paused(True)
    time.sleep(0.05)
    rows = engine.export_csv(tmp_path / "after_reset.csv")
    snap, _ = engine.frame()
    # Only the post-reset session remains: about 1 kHz sampling of snap.t
    # seconds, far less than the pre-reset history would add.
    assert rows == pytest.approx(snap.t * 1000, abs=100)
