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

from rhc_demo.control_panel import ControlPanel
from rhc_demo.curves import CurveWorker
from rhc_demo.engine import LiveEngine
from rhc_demo.params import Params
from rhc_demo.phase_view import PhaseView
from rhc_demo.robot_view import RobotView

if TYPE_CHECKING:
    from rhc_demo.replay import ReplaySource
    from rhc_demo.state import Snapshot

FRAME_MS = 16
CURVE_DEBOUNCE_MS = 150
# Recompute replay curves when logged guides move by more than this.
CURVE_PARAM_TOL = 1e-4


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
        root = QHBoxLayout()
        root.addWidget(self.phase_view, 5)
        root.addWidget(self.robot_view, 5)
        root.addWidget(self.panel, 3)
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
        # The panel's checkbox state is authoritative for the history mode.
        self.phase_view.set_trail_mode(self.panel.trail_mode_enabled())
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
        p.resetRequested.connect(self._on_reset)
        p.trailModeChanged.connect(self.phase_view.set_trail_mode)
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
        self.robot_view.set_snapshot(snap)
        self.panel.update_readout(snap)
        if not self.interactive:
            self.panel.reflect_snapshot(snap)
            self._maybe_refresh_replay_curves(snap)
        result = self._curve_worker.take_result()
        if result is not None:
            seeds, curves = result
            self.phase_view.set_curves(curves, seeds)
        return snap

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
            soft_landing=snap.soft_landing,
        )
        prev = self._curve_params
        if prev is None or any(
            abs(getattr(logged, f) - getattr(prev, f)) > CURVE_PARAM_TOL
            for f in ("za", "zm", "zb", "rho", "k", "soft_landing")
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
