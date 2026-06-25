"""Main window: wires the driver to the biped, phase portrait and controls."""

from __future__ import annotations

from collections import deque

import numpy as np
from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import QHBoxLayout, QMainWindow, QVBoxLayout, QWidget

from rhc_demo.biped_widget import BipedWidget
from rhc_demo.controls import ControlsPanel
from rhc_demo.driver import Driver, Params
from rhc_demo.field import build_field
from rhc_demo.phase_widget import PhaseWidget

FRAME_MS = 16
TRAIL_LEN = 1500
FIELD_DEBOUNCE_MS = 150
FIELD_DURATION = 1.0
FIELD_DT = 4e-4
DISTURB_FORCE = 120.0  # N, applied upward for a short window
DISTURB_DURATION = 0.05  # s


class MainWindow(QMainWindow):
    """The demo window."""

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Dynamics Morphing — Standing ↔ Hopping")
        # Start at the crouch bottom, which lies on the hopping limit cycle, so
        # the default rho=1 mode begins hopping immediately rather than sitting
        # at the unstable standing equilibrium.
        params = Params()
        self.driver = Driver(params, dt=0.001, z0=params.zb, vz0=0.0)
        self._trail_z: deque[float] = deque(maxlen=TRAIL_LEN)
        self._trail_vz: deque[float] = deque(maxlen=TRAIL_LEN)

        self.biped = BipedWidget()
        self.phase = PhaseWidget()
        self.controls = ControlsPanel(self.driver.params)
        self._build_layout()
        self._connect_controls()

        self._field_timer = QTimer(self)
        self._field_timer.setSingleShot(True)
        self._field_timer.setInterval(FIELD_DEBOUNCE_MS)
        self._field_timer.timeout.connect(self.recompute_field)

        self._timer = QTimer(self)
        self._timer.setInterval(FRAME_MS)
        self._timer.timeout.connect(self.tick)

        self.recompute_field()
        self._timer.start()

    def _build_layout(self) -> None:
        viz_top = QHBoxLayout()
        viz_top.addWidget(self.biped, 1)
        viz_top.addWidget(self.phase, 2)
        viz = QVBoxLayout()
        viz.addLayout(viz_top)
        root = QHBoxLayout()
        root.addWidget(self.controls, 0)
        root.addLayout(viz, 1)
        central = QWidget()
        central.setLayout(root)
        self.setCentralWidget(central)

    def _connect_controls(self) -> None:
        c = self.controls
        c.rhoChanged.connect(self._on_rho)
        c.zaChanged.connect(self._on_za)
        c.zmChanged.connect(self._on_zm)
        c.zbChanged.connect(self._on_zb)
        c.zhChanged.connect(self._on_zh)
        c.kChanged.connect(self._on_k)
        c.massChanged.connect(self._on_mass)
        c.disturbanceRequested.connect(self._on_disturb)
        c.resetRequested.connect(self._on_reset)

    # -- animation --
    def tick(self) -> None:
        substeps = max(1, round(FRAME_MS / 1000.0 / self.driver.dt))
        self.driver.step(substeps)
        snap = self.driver.snapshot()
        self._trail_z.append(snap.z)
        self._trail_vz.append(snap.vz)
        self.biped.set_state(snap.z, contact=snap.contact, zh=self.driver.params.zh, za=self.driver.params.za)
        self.phase.update_live(np.fromiter(self._trail_z, dtype=float), np.fromiter(self._trail_vz, dtype=float))
        self.controls.set_readout(self._readout(snap))

    @staticmethod
    def _readout(snap) -> str:
        mode = snap.phase.name.lower()
        return (
            f"t = {snap.t:5.2f} s\n"
            f"z = {snap.z:.3f} m,  vz = {snap.vz:+.3f} m/s\n"
            f"phase: {mode},  {'contact' if snap.contact else 'flight'}\n"
            f"fz = {snap.fz:6.1f} N,  hops: {snap.n}\n"
            f"adjusted: za={snap.param_za:.3f} zm={snap.param_zm:.3f} zb={snap.param_zb:.3f}"
        )

    def recompute_field(self) -> None:
        field = build_field(self.driver.params, duration=FIELD_DURATION, dt=FIELD_DT)
        self.phase.set_field(field, self.driver.params)

    def _schedule_field(self) -> None:
        self._field_timer.start()

    # -- control slots --
    def _on_rho(self, v: float) -> None:
        self.driver.set_rho(v)
        self._schedule_field()

    def _on_za(self, v: float) -> None:
        self.driver.set_za(v)
        self._schedule_field()

    def _on_zm(self, v: float) -> None:
        self.driver.set_zm(v)
        self._schedule_field()

    def _on_zb(self, v: float) -> None:
        self.driver.set_zb(v)
        self._schedule_field()

    def _on_zh(self, v: float) -> None:
        self.driver.set_zh(v)
        self._schedule_field()

    def _on_k(self, v: float) -> None:
        self.driver.set_k(v)
        self._schedule_field()

    def _on_mass(self, v: float) -> None:
        self.driver.set_mass(v)
        self._schedule_field()

    def _on_disturb(self) -> None:
        self.driver.apply_disturbance(DISTURB_FORCE, DISTURB_DURATION)

    def _on_reset(self) -> None:
        self.driver.reset()
        self._trail_z.clear()
        self._trail_vz.clear()
        self.recompute_field()
