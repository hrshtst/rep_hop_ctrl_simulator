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

    from rhc_demo.robot_view import FE_N_PER_PX

    view.mousePressEvent(ev(QMouseEvent.Type.MouseButtonPress, QPointF(200, 300)))
    # Diagonal drag: only the vertical component may matter.
    view.mouseMoveEvent(ev(QMouseEvent.Type.MouseMove, QPointF(150, 250)))
    view.mouseReleaseEvent(ev(QMouseEvent.Type.MouseButtonRelease, QPointF(150, 250)))
    assert forces[0] == pytest.approx(50 * FE_N_PER_PX)  # 50 px vertical component
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


def test_phase_view_limit_cycle_overlay(qapp):
    from rhc_demo.phase_view import PhaseView
    from rhc_demo.state import Snapshot

    view = PhaseView()
    view.resize(400, 400)
    view.set_snapshot(Snapshot(z=0.26, vz=0.0, zh=0.26, za=0.28, zb=0.23))
    without_cycle = view.grab().toImage()
    theta = np.linspace(0.0, 2 * np.pi, 100)
    ellipse = (0.26 + 0.015 * np.cos(theta), 0.66 * np.sin(theta))
    view.set_limit_cycle(ellipse, ellipse)
    with_cycle = view.grab().toImage()
    assert with_cycle != without_cycle
    # The panel toggle hides every orbit overlay.
    view.set_show_cycles(False)
    assert view.grab().toImage() == without_cycle
    view.set_show_cycles(True)
    view.set_limit_cycle(None, None)
    assert view.grab().toImage() == without_cycle


def test_phase_view_cushion_ellipse_overlay(qapp):
    from rhc_demo.phase_view import PhaseView
    from rhc_demo.state import Snapshot

    view = PhaseView()
    view.resize(400, 400)
    view.set_snapshot(Snapshot(z=0.26, vz=0.0, zh=0.26, za=0.28, zb=0.23))
    plain = view.grab().toImage()
    theta = np.linspace(0.0, 2 * np.pi, 100)
    view.set_cushion_ellipse((0.27 + 0.04 * np.cos(theta), 1.2 * np.sin(theta)))
    assert view.grab().toImage() != plain
    view.set_cushion_ellipse(None)
    assert view.grab().toImage() == plain


def test_cushion_ellipse_only_when_apex_boosted(qapp):
    from rhc_demo.main_window import MainWindow
    from rhc_demo.state import Snapshot

    base = {
        "za": 0.28,
        "zh": 0.26,
        "k": 4.0,
        "q_scale": 1.0,
        "soft_landing": True,
        "p_rho": 1.0,
        "p_zb": 0.23,
    }
    # Steady state: morphed apex equals the command, no cushion orbit.
    assert MainWindow._cushion_ellipse(Snapshot(**base, p_za=0.28, p_zm=0.255)) is None
    # Landing from a boosted apex 0.35: the enlarged ellipse appears with
    # the paper's adjusted center zm' = (za' + zb - (za'-zh)^2/(za'-zb))/2
    # and passes through the landing energy (top at 2 zm' - zb).
    zm_prime = 0.5 * (0.35 + 0.23 - (0.35 - 0.26) ** 2 / (0.35 - 0.23))
    boosted = MainWindow._cushion_ellipse(Snapshot(**base, p_za=0.35, p_zm=zm_prime))
    assert boosted is not None
    z, _ = boosted
    assert z.max() == pytest.approx(2 * zm_prime - 0.23, abs=1e-6)
    # Disabled soft landing never shows a cushion orbit.
    off = dict(base, soft_landing=False)
    assert MainWindow._cushion_ellipse(Snapshot(**off, p_za=0.35, p_zm=zm_prime)) is None


def test_param_reset_buttons(qapp):
    from rhc_demo.control_panel import ControlPanel
    from rhc_demo.params import Params

    panel = ControlPanel(Params())
    changes = []
    panel.paramChanged.connect(lambda name, value: changes.append((name, value)))
    # rho toggles between the extremes.
    panel._sliders["rho"]._reset_btn.click()
    assert panel.params.rho == 1.0
    panel._sliders["rho"]._reset_btn.click()
    assert panel.params.rho == 0.0
    # Other parameters snap back to the paper default.
    panel._sliders["za"].valueChanged.emit(0.40)
    assert panel.params.za == pytest.approx(0.40)
    panel._sliders["za"]._reset_btn.click()
    assert panel.params.za == pytest.approx(Params().za)
    assert changes[-1] == ("za", pytest.approx(Params().za))


def test_limit_cycle_toggle_defaults_on(qapp):
    from rhc_demo.control_panel import ControlPanel
    from rhc_demo.params import Params
    from rhc_demo.phase_view import PhaseView

    assert PhaseView()._show_cycles is True
    panel = ControlPanel(Params())
    assert panel.limit_cycles_enabled() is True
    states = []
    panel.limitCyclesToggled.connect(states.append)
    panel._limit_cycles.setChecked(False)
    assert states == [False]


def test_speed_radios_emit_selected_factor(qapp):
    from rhc_demo.control_panel import ControlPanel
    from rhc_demo.params import Params

    panel = ControlPanel(Params())
    received = []
    panel.speedChanged.connect(received.append)
    panel._speed_buttons[0.5].setChecked(True)
    panel._speed_buttons[0.25].setChecked(True)
    assert received == [0.5, 0.25]


def test_q_slider_present_below_k(qapp):
    from rhc_demo.control_panel import ControlPanel
    from rhc_demo.params import Params

    panel = ControlPanel(Params())
    names = list(panel._sliders)
    assert names.index("q_scale") == names.index("k") + 1


def test_fading_trail_is_the_default(qapp):
    from rhc_demo.control_panel import ControlPanel
    from rhc_demo.params import Params
    from rhc_demo.phase_view import PhaseView

    assert PhaseView()._trail_mode is True
    assert ControlPanel(Params()).trail_mode_enabled() is True


def test_window_syncs_trail_mode_from_panel(replay_window):
    assert replay_window.phase_view._trail_mode is replay_window.panel.trail_mode_enabled()
