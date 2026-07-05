"""Offscreen widget smoke tests: window wiring and frame rendering."""

from __future__ import annotations

import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import numpy as np
import pytest

import rhc
from rhc_demo.csv_io import concat_records
from rhc_demo.replay import ReplaySource


@pytest.fixture(scope="module")
def qapp():
    from PyQt6.QtWidgets import QApplication

    return QApplication.instance() or QApplication([])


@pytest.fixture
def replay_window(qapp):
    from rhc_demo.main_window import MainWindow

    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    data = concat_records([sim.advance(5000, 1e-4, 10)])
    window = MainWindow(ReplaySource(data))
    window._timer.stop()
    window.resize(1280, 720)
    yield window
    window.close()


def test_replay_window_renders_and_reflects(replay_window):
    window = replay_window
    window.source.set_paused(True)
    window.source.seek_time(0.25)
    snap = window.render_frame()
    assert snap.t == pytest.approx(0.25, abs=0.01)
    # Logged parameters are mirrored onto the (disabled) panel.
    assert window.panel._sliders["za"]._slider.value() > 0
    assert not window.panel._sliders["za"]._slider.isEnabled()
    pixmap = window.grab()
    assert pixmap.width() > 0


def test_grabbed_frame_changes_over_time(replay_window):
    window = replay_window
    window.source.set_paused(True)
    window.source.seek_time(0.0)
    window.render_frame()
    img0 = window.grab().toImage()
    window.source.seek_time(0.4)
    window.render_frame()
    img1 = window.grab().toImage()
    assert img0 != img1


def test_interactive_window_lifecycle(qapp):
    from rhc_demo.engine import LiveEngine
    from rhc_demo.main_window import MainWindow

    window = MainWindow(LiveEngine())
    window._timer.stop()
    window.resize(1280, 720)
    import time

    time.sleep(0.2)  # let physics produce samples
    snap = window.render_frame()
    assert snap.t > 0.0
    assert window.grab().width() > 0
    window.close()


def test_phase_view_trail_toggle(qapp):
    from rhc_demo.phase_view import PhaseView
    from rhc_demo.state import Snapshot

    view = PhaseView()
    view.resize(400, 400)
    view.push_samples([(np.linspace(0.23, 0.28, 100), np.linspace(-1, 1, 100))])
    view.set_snapshot(Snapshot(z=0.26, vz=0.1, zh=0.26, za=0.28, zb=0.23))
    view.set_trail_mode(True)
    view.grab()
    view.set_trail_mode(False)
    view.grab()


def test_robot_view_drag_emits_vertical_force(qapp):
    from PyQt6.QtCore import QPointF, Qt
    from PyQt6.QtGui import QMouseEvent

    from rhc_demo.robot_view import RobotView
    from rhc_demo.state import Snapshot

    view = RobotView(interactive=True)
    view.resize(400, 400)
    view.set_snapshot(Snapshot(z=0.2575, vz=0.0, zh=0.26, za=0.28, zb=0.23, phase=1))
    forces = []
    view.feChanged.connect(forces.append)

    def ev(kind, pos, button=Qt.MouseButton.LeftButton):
        return QMouseEvent(kind, pos, button, button, Qt.KeyboardModifier.NoModifier)

    view.mousePressEvent(ev(QMouseEvent.Type.MouseButtonPress, QPointF(200, 300)))
    # Diagonal drag: only the vertical component may matter.
    view.mouseMoveEvent(ev(QMouseEvent.Type.MouseMove, QPointF(150, 250)))
    view.mouseReleaseEvent(ev(QMouseEvent.Type.MouseButtonRelease, QPointF(150, 250)))
    assert forces[0] == pytest.approx(50 * 3.0)  # 50 px up * 3 N/px
    assert forces[-1] == 0.0


def test_phase_view_seed_overlay(qapp):
    import numpy as np

    from rhc_demo.phase_view import PhaseView
    from rhc_demo.state import Snapshot

    view = PhaseView(show_seeds=True)
    view.resize(400, 400)
    view.set_snapshot(Snapshot(z=0.26, vz=0.0, zh=0.26, za=0.28, zb=0.23))
    curves = [(np.linspace(0.20, 0.30, 50), np.linspace(-1.0, 1.0, 50))]
    view.set_curves(curves, seeds=[(0.20, -1.0), (0.30, 1.0)])
    with_seeds = view.grab().toImage()
    view.set_show_seeds(False)
    without_seeds = view.grab().toImage()
    assert with_seeds != without_seeds
