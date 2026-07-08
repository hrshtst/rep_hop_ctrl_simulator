"""Main window: Phase Portrait | Robot View | Control Panel.

The window is a pure consumer: on every UI frame it pulls a snapshot and
the new samples from its source (live engine or replay) and pushes them
into the widgets. All physics runs on the engine's own thread; solution
curves are recomputed on a background worker, debounced, and picked up
by the frame timer — the GUI thread never blocks on simulation work.
"""

from __future__ import annotations

import math
from typing import TYPE_CHECKING

from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import QFileDialog, QHBoxLayout, QMainWindow, QWidget

import rhc
from rhc_demo.control_panel import ControlPanel
from rhc_demo.curves import CurveWorker
from rhc_demo.engine import LiveEngine
from rhc_demo.params import Params
from rhc_demo.phase_view import PhaseView
from rhc_demo.robot_view import RobotView

if TYPE_CHECKING:
    import numpy as np

    from rhc_demo.replay import ReplaySource
    from rhc_demo.state import Snapshot

FRAME_MS = 16
CURVE_DEBOUNCE_MS = 150
# Recompute replay curves when logged guides move by more than this.
CURVE_PARAM_TOL = 1e-4
# Apex boost (m) beyond the commanded za before the transient
# soft-landing cushion ellipse is drawn; filters steady-state jitter.
CUSHION_TOL = 1e-3


class MainWindow(QMainWindow):
    """Three-panel demo window driven by a live engine or a replay source."""

    def __init__(self, source: LiveEngine | ReplaySource, *, show_seeds: bool = False) -> None:
        super().__init__()
        self.source = source
        self.interactive = isinstance(source, LiveEngine)
        title_mode = "interactive" if self.interactive else "replay"
        self.setWindowTitle(f"Dynamics Morphing — standing ↔ hopping ({title_mode})")

        params = source.params if self.interactive else Params()
        self.phase_view = PhaseView(show_seeds=show_seeds)
        self.robot_view = RobotView(interactive=self.interactive)
        self.panel = ControlPanel(params, interactive=self.interactive)
        # The phase portrait gets the widest pane so it renders close to
        # a square; its z-range upper limit is widened to match. The
        # panel's share fits the per-slider reset buttons.
        root = QHBoxLayout()
        root.addWidget(self.phase_view, 7)
        root.addWidget(self.robot_view, 5)
        root.addWidget(self.panel, 4)
        central = QWidget()
        central.setLayout(root)
        self.setCentralWidget(central)

        self._curve_worker = CurveWorker()
        self._curve_params: Params | None = None
        self._curve_debounce = QTimer(self)
        self._curve_debounce.setSingleShot(True)
        self._curve_debounce.setInterval(CURVE_DEBOUNCE_MS)
        self._curve_debounce.timeout.connect(self._request_curves)

        self._connect_panel()
        # The panel's checkbox state is authoritative for the view options.
        self.phase_view.set_trail_mode(self.panel.trail_mode_enabled())
        self.phase_view.set_show_cycles(self.panel.limit_cycles_enabled())
        if self.interactive:
            self.robot_view.feChanged.connect(self.source.set_fe)

        self._timer = QTimer(self)
        self._timer.setInterval(FRAME_MS)
        self._timer.timeout.connect(self._tick)

        self.source.start()
        self._request_curves()
        self._timer.start()

    # -- wiring ----------------------------------------------------------------
    def _connect_panel(self) -> None:
        p = self.panel
        p.pauseToggled.connect(lambda paused: self.source.set_paused(paused))
        p.stepRequested.connect(lambda: self.source.request_step())
        if not self.interactive:
            # Rewinding needs recorded samples: replay sources only.
            p.stepBackRequested.connect(lambda: self.source.request_step_back())
        p.resetRequested.connect(self._on_reset)
        p.trailModeChanged.connect(self.phase_view.set_trail_mode)
        p.limitCyclesToggled.connect(self.phase_view.set_show_cycles)
        p.speedChanged.connect(lambda speed: self.source.set_speed(speed))
        if self.interactive:
            p.paramChanged.connect(self._on_param_changed)
            p.softLandingChanged.connect(lambda on: self._on_param_changed("soft_landing", on))
            p.exportRequested.connect(self._on_export)

    # -- frame loop ---------------------------------------------------------------
    def _tick(self) -> None:
        self.render_frame()

    def render_frame(self) -> Snapshot:
        """Pull one frame from the source and refresh every widget."""
        snap, chunks = self.source.frame()
        if chunks:
            self.phase_view.push_samples(chunks)
        self.phase_view.set_snapshot(snap)
        self.phase_view.set_cushion_ellipse(self._cushion_ellipse(snap))
        self.robot_view.set_snapshot(snap)
        self.panel.update_readout(snap)
        if not self.interactive:
            self.panel.reflect_snapshot(snap)
            self._maybe_refresh_replay_curves(snap)
        portrait = self._curve_worker.take_result()
        if portrait is not None:
            self.phase_view.set_curves(portrait.curves, portrait.seeds)
            self.phase_view.set_limit_cycle(portrait.cycle, portrait.ellipse)
        return snap

    @staticmethod
    def _cushion_ellipse(snap: Snapshot) -> tuple[np.ndarray, np.ndarray] | None:
        """Build the transient soft-landing orbit from the morphed parameters.

        When a disturbance makes the robot land from an apex above the
        commanded za, the soft-landing layer temporarily enlarges the
        limit cycle to pass through the actual landing state (paper
        Fig. 8); its geometry is exactly the controller's morphed
        parameters. Returns None while no enlarged orbit is active.
        """
        needed = (snap.za, snap.zh, snap.k, snap.p_za, snap.p_zm, snap.p_zb, snap.p_rho)
        if not snap.soft_landing or any(math.isnan(v) for v in needed):
            return None
        if snap.p_za <= snap.za + CUSHION_TOL:
            return None
        q_scale = snap.q_scale if not math.isnan(snap.q_scale) else 1.0
        z, vz = rhc.stance_ellipse(
            zh=snap.zh,
            zm=snap.p_zm,
            zb=snap.p_zb,
            rho=snap.p_rho,
            k=snap.k,
            q_scale=q_scale,
        )
        return (z, vz) if len(z) else None

    # -- solution curves ---------------------------------------------------------
    def _request_curves(self) -> None:
        params = self.panel.params
        self._curve_params = params
        self._curve_worker.request(params)

    def _maybe_refresh_replay_curves(self, snap: Snapshot) -> None:
        if math.isnan(snap.za) or math.isnan(snap.zh):
            return
        logged = Params(
            za=snap.za,
            zh=snap.zh,
            zm=snap.zm,
            zb=snap.zb,
            rho=snap.rho,
            k=snap.k,
            q_scale=snap.q_scale if not math.isnan(snap.q_scale) else 1.0,
            soft_landing=snap.soft_landing,
        )
        prev = self._curve_params
        if prev is None or any(
            abs(getattr(logged, f) - getattr(prev, f)) > CURVE_PARAM_TOL
            for f in ("za", "zm", "zb", "rho", "k", "q_scale", "soft_landing")
        ):
            self._curve_params = logged
            self._curve_worker.request(logged)

    # -- slots ---------------------------------------------------------------------
    def _on_param_changed(self, name: str, value: float | bool) -> None:
        self.source.set_param(name, value)
        self._curve_debounce.start()

    def _on_reset(self) -> None:
        self.source.reset()
        self.phase_view.clear_history()

    def _on_export(self) -> None:
        path, _ = QFileDialog.getSaveFileName(self, "Export session CSV", "session.csv", "CSV files (*.csv)")
        if path:
            rows = self.source.export_csv(path)
            self.statusBar().showMessage(f"Exported {rows} samples to {path}", 5000)

    # -- teardown --------------------------------------------------------------------
    def closeEvent(self, event) -> None:
        self._timer.stop()
        self._curve_worker.close()
        self.source.close()
        super().closeEvent(event)
