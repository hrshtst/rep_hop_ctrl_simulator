"""Control panel: parameter sliders, toggles, and execution controls.

Sliders cover the morphing parameter rho and the target heights za, zm,
zb; slider logic clamps requested values through
:func:`rhc_demo.params.clamp_param` so the kinematic constraints
``zb < zm < zh`` and ``zb < za`` are never violated. zh is a fixed robot
constant and is displayed, not adjustable. Execution controls (pause,
reset, step) work in both replay and interactive modes; in replay mode
the parameter widgets are disabled and mirror the logged values.
"""

from __future__ import annotations

import math
from dataclasses import replace
from typing import TYPE_CHECKING

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QCheckBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QPushButton,
    QSlider,
    QVBoxLayout,
    QWidget,
)

from rhc_demo.params import K_RANGE, ZA_RANGE, ZB_RANGE, ZM_RANGE, Params, clamp_param
from rhc_demo.state import Snapshot, fmt

if TYPE_CHECKING:
    from collections.abc import Callable

SLIDER_TICKS = 1000


class _ParamSlider(QWidget):
    """A labelled float slider mapping to [lo, hi] with SLIDER_TICKS steps."""

    valueChanged = pyqtSignal(float)

    def __init__(self, label: str, lo: float, hi: float, value: float, unit: str = "m") -> None:
        super().__init__()
        self._lo = lo
        self._hi = hi
        self._unit = unit
        self._label = QLabel()
        self._slider = QSlider(Qt.Orientation.Horizontal)
        self._slider.setRange(0, SLIDER_TICKS)
        self._name = label
        self._updating = False
        self.set_value(value)
        self._slider.valueChanged.connect(self._on_slider)
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(1)
        layout.addWidget(self._label)
        layout.addWidget(self._slider)
        self.setLayout(layout)

    def _to_float(self, ticks: int) -> float:
        return self._lo + (self._hi - self._lo) * ticks / SLIDER_TICKS

    def _to_ticks(self, value: float) -> int:
        return round((value - self._lo) / (self._hi - self._lo) * SLIDER_TICKS)

    def _on_slider(self, ticks: int) -> None:
        if not self._updating:
            self.valueChanged.emit(self._to_float(ticks))

    def set_value(self, value: float) -> None:
        """Move the slider programmatically without emitting valueChanged."""
        if math.isnan(value):
            return
        self._updating = True
        self._slider.setValue(self._to_ticks(value))
        self._updating = False
        digits = 3 if self._unit == "m" else 2
        self._label.setText(f"{self._name} = {value:.{digits}f} {self._unit}".rstrip())

    def setEnabled(self, enabled: bool) -> None:
        self._slider.setEnabled(enabled)


class ControlPanel(QWidget):
    """User inputs and simulation execution controls."""

    paramChanged = pyqtSignal(str, float)
    softLandingChanged = pyqtSignal(bool)
    trailModeChanged = pyqtSignal(bool)
    pauseToggled = pyqtSignal(bool)
    stepRequested = pyqtSignal()
    resetRequested = pyqtSignal()
    exportRequested = pyqtSignal()

    def __init__(self, params: Params, *, interactive: bool = True) -> None:
        super().__init__()
        self.params = params
        self._interactive = interactive
        self.setMinimumWidth(240)

        root = QVBoxLayout()
        root.addWidget(self._make_param_group(params))
        root.addWidget(self._make_toggle_group(params))
        root.addWidget(self._make_execution_group())
        self._readout = QLabel()
        self._readout.setTextFormat(Qt.TextFormat.PlainText)
        self._readout.setStyleSheet("font-family: monospace; font-size: 11px;")
        root.addWidget(self._readout)
        root.addStretch(1)
        self.setLayout(root)

        if not interactive:
            self.set_parameters_enabled(False)

    # -- construction ---------------------------------------------------------
    def _make_param_group(self, p: Params) -> QGroupBox:
        group = QGroupBox("Control parameters")
        grid = QVBoxLayout()
        self._sliders: dict[str, _ParamSlider] = {}
        specs: list[tuple[str, str, tuple[float, float], float, str]] = [
            ("rho", "ρ̃  (0: stand, 1: hop)", (0.0, 1.0), p.rho, ""),
            ("za", "z̃_a  target apex", ZA_RANGE, p.za, "m"),
            ("zm", "z̃_m  standing height", ZM_RANGE, p.zm, "m"),
            ("zb", "z̃_b  lower limit", ZB_RANGE, p.zb, "m"),
            ("k", "k  convergence gain", K_RANGE, p.k, ""),
        ]
        for name, label, (lo, hi), value, unit in specs:
            slider = _ParamSlider(label, lo, hi, value, unit)
            slider.valueChanged.connect(self._make_param_handler(name))
            self._sliders[name] = slider
            grid.addWidget(slider)
        self._zh_label = QLabel(f"z_h = {p.zh:.3f} m (robot constant)")
        grid.addWidget(self._zh_label)
        group.setLayout(grid)
        return group

    def _make_param_handler(self, name: str) -> Callable[[float], None]:
        def handler(value: float) -> None:
            clamped = clamp_param(self.params, name, value)
            self.params = replace(self.params, **{name: clamped})
            self._sliders[name].set_value(clamped)
            self.paramChanged.emit(name, clamped)

        return handler

    def _make_toggle_group(self, p: Params) -> QGroupBox:
        group = QGroupBox("Toggles")
        box = QVBoxLayout()
        self._soft_landing = QCheckBox("Soft-landing adjustment")
        self._soft_landing.setChecked(p.soft_landing)
        self._soft_landing.toggled.connect(self.softLandingChanged.emit)
        box.addWidget(self._soft_landing)
        self._trail_mode = QCheckBox("Fading COM trail (vs full history)")
        # Checked by default: the full-history line is ambiguous next to
        # the gray solution curves.
        self._trail_mode.setChecked(True)
        self._trail_mode.toggled.connect(self.trailModeChanged.emit)
        box.addWidget(self._trail_mode)
        group.setLayout(box)
        return group

    def _make_execution_group(self) -> QGroupBox:
        group = QGroupBox("Execution")
        grid = QGridLayout()
        self._pause_btn = QPushButton("Pause")
        self._pause_btn.setCheckable(True)
        self._pause_btn.toggled.connect(self._on_pause_toggled)
        self._step_btn = QPushButton("Step")
        self._step_btn.setEnabled(False)
        self._step_btn.clicked.connect(self.stepRequested.emit)
        self._reset_btn = QPushButton("Reset")
        self._reset_btn.clicked.connect(self.resetRequested.emit)
        grid.addWidget(self._pause_btn, 0, 0)
        grid.addWidget(self._step_btn, 0, 1)
        grid.addWidget(self._reset_btn, 1, 0)
        if self._interactive:
            export_btn = QPushButton("Export CSV…")
            export_btn.clicked.connect(self.exportRequested.emit)
            grid.addWidget(export_btn, 1, 1)
        group.setLayout(grid)
        return group

    def _on_pause_toggled(self, paused: bool) -> None:
        self._pause_btn.setText("Resume" if paused else "Pause")
        self._step_btn.setEnabled(paused)
        self.pauseToggled.emit(paused)

    def trail_mode_enabled(self) -> bool:
        return self._trail_mode.isChecked()

    # -- updates from the frame loop --------------------------------------------
    def set_parameters_enabled(self, enabled: bool) -> None:
        for slider in self._sliders.values():
            slider.setEnabled(enabled)
        self._soft_landing.setEnabled(enabled)

    def reflect_snapshot(self, snap: Snapshot) -> None:
        """In replay mode, mirror the logged parameter values on the widgets."""
        for name in ("rho", "za", "zm", "zb", "k"):
            self._sliders[name].set_value(getattr(snap, name))
        if not math.isnan(snap.zh):
            self._zh_label.setText(f"z_h = {snap.zh:.3f} m (robot constant)")
        self._soft_landing.setChecked(snap.soft_landing)

    def update_readout(self, snap: Snapshot) -> None:
        status = "finished" if snap.finished else ("running" if snap.playing else "paused")
        self._readout.setText(
            f"t     = {snap.t:8.3f} s   [{status}]\n"
            f"z     = {fmt(snap.z)} m\n"
            f"ż     = {fmt(snap.vz)} m/s\n"
            f"phase = {snap.phase_name}"
            f" ({'contact' if snap.contact else 'flight'})\n"
            f"f_z   = {snap.fz:7.1f} N\n"
            f"f_e   = {snap.fe:+7.1f} N\n"
            f"hops  = {snap.hops}\n"
            f"morphed:\n"
            f"  ρ   = {fmt(snap.p_rho, 2)}\n"
            f"  z_a = {fmt(snap.p_za)} m\n"
            f"  z_m = {fmt(snap.p_zm)} m\n"
            f"  z_b = {fmt(snap.p_zb)} m",
        )

    def sync_pause_state(self, *, paused: bool) -> None:
        """Keep the pause button consistent when pausing is triggered elsewhere."""
        if self._pause_btn.isChecked() != paused:
            self._pause_btn.blockSignals(True)  # noqa: FBT003
            self._pause_btn.setChecked(paused)
            self._pause_btn.setText("Resume" if paused else "Pause")
            self._step_btn.setEnabled(paused)
            self._pause_btn.blockSignals(False)  # noqa: FBT003
