"""Headless rendering tests: frame counts, playback speed, and panel trim."""

from __future__ import annotations

import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import pytest

import rhc
from rhc_demo.csv_io import concat_records
from rhc_demo.headless import render_frames
from rhc_demo.replay import ReplaySource


@pytest.fixture(scope="module")
def qapp():
    from PyQt6.QtWidgets import QApplication

    return QApplication.instance() or QApplication([])


@pytest.fixture
def data():
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    return concat_records([sim.advance(3000, 1e-4, 10)])  # 0.3 s at 1 kHz


def _expected_frames(source: ReplaySource, fps: int, speed: float) -> int:
    # render_frames: one frame per 1/fps of *video* time over
    # duration/speed seconds, inclusive of t = 0.
    return int(source.duration / speed * fps) + 1


def test_render_frames_at_real_time(qapp, data, tmp_path):
    source = ReplaySource(data)
    frames = render_frames(source, tmp_path, fps=10, size=(480, 320))
    assert len(frames) == _expected_frames(source, 10, 1.0)
    assert all(p.exists() for p in frames)


def test_speed_scales_the_rendered_duration(qapp, data, tmp_path):
    # Half speed doubles the video duration, hence the frame count;
    # double speed halves it.
    slow_src = ReplaySource(data)
    slow = render_frames(slow_src, tmp_path / "slow", fps=10, size=(480, 320), speed=0.5)
    assert len(slow) == _expected_frames(slow_src, 10, 0.5)
    fast_src = ReplaySource(data)
    fast = render_frames(fast_src, tmp_path / "fast", fps=10, size=(480, 320), speed=2.0)
    assert len(fast) == _expected_frames(fast_src, 10, 2.0)
    assert len(slow) == pytest.approx(4 * len(fast), abs=2)


def test_headless_window_hides_playback_speed_radios(qapp, data):
    from rhc_demo.main_window import MainWindow

    window = MainWindow(ReplaySource(data), headless=True)
    window._timer.stop()
    assert not hasattr(window.panel, "_speed_buttons")
    window.close()


def test_headless_frames_keep_the_interactive_look(qapp, data):
    from PyQt6.QtWidgets import QPushButton

    from rhc_demo.main_window import MainWindow

    window = MainWindow(ReplaySource(data), headless=True)
    window._timer.stop()
    panel = window.panel
    # Unlike replay mode, nothing is grayed out...
    assert panel._soft_landing.isEnabled()
    assert all(s._slider.isEnabled() for s in panel._sliders.values())
    # ...the Export button is present like in interactive mode...
    labels = [b.text() for b in panel.findChildren(QPushButton)]
    assert "Export CSV…" in labels
    # ...and the readout reads as a running session, not a paused replay.
    window.source.set_paused(True)
    window.source.seek_time(0.1)
    window.render_frame()
    assert "running" in panel._readout.text()
    window.close()


def test_replay_window_still_grays_parameters(qapp, data):
    from rhc_demo.main_window import MainWindow

    window = MainWindow(ReplaySource(data))
    window._timer.stop()
    assert not window.panel._soft_landing.isEnabled()
    window.close()
