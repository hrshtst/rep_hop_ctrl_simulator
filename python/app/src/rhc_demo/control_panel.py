"""Control panel: parameter sliders, toggles, and execution controls.

Sliders cover the morphing parameter rho, the target heights za, zm,
zb, and the controller gains k and q; slider logic clamps requested
values through :func:`rhc_demo.params.clamp_param` so the kinematic
constraints ``zb < zm < zh`` and ``zb < za`` are never violated. zh is
a fixed robot constant and is displayed, not adjustable. Execution
controls (pause, reset, step) and the playback-speed radio buttons work
in both replay and interactive modes; in replay mode the parameter
widgets are disabled and mirror the logged values. Headless mode also
mirrors the log but keeps every widget looking interactive (and drops
the playback-speed radios, which the CLI fixes), so rendered videos
show exactly what an interactive user would see.
"""

from __future__ import annotations

import math
from dataclasses import replace
from typing import TYPE_CHECKING

import qtawesome as qta
from PyQt6.QtCore import QSize, Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QCheckBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QRadioButton,
    QSlider,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

from rhc_demo.params import K_RANGE, Q_SCALE_RANGE, ZA_RANGE, ZB_RANGE, ZM_RANGE, Params, clamp_param
from rhc_demo.state import Snapshot, fmt

if TYPE_CHECKING:
    from collections.abc import Callable

    from PyQt6.QtGui import QIcon

SLIDER_TICKS = 1000
PLAYBACK_SPEEDS = (1.0, 0.75, 0.5, 0.25)
PARAM_SPACING = 12  # px between the sliders so their labels stay clear
ICON_COLOR = "#404040"  # transport-button glyphs, matching the app grays

# Per-slider reset targets: the paper defaults.
_DEFAULTS = Params()


def _icon(name: str) -> QIcon:
    # qtawesome renders from bundled fonts, so the glyphs look the same
    # on Wayland, WSLg, and the offscreen platform used for videos.
    return qta.icon(name, color=ICON_COLOR)


class _ParamSlider(QWidget):
    """A labelled float slider mapping to [lo, hi] with SLIDER_TICKS steps.

    A small reset button sits next to the slider; the panel decides
    what value it snaps to.
    """

    valueChanged = pyqtSignal(float)
    resetClicked = pyqtSignal()

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
        self._reset_btn = QToolButton()
        self._reset_btn.setText("↺")
        self._reset_btn.setToolTip("Reset to default")
        self._reset_btn.clicked.connect(self.resetClicked.emit)
        row = QHBoxLayout()
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(4)
        row.addWidget(self._slider)
        row.addWidget(self._reset_btn)
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(1)
        layout.addWidget(self._label)
        layout.addLayout(row)
        self.setLayout(layout)

    def set_reset_tooltip(self, text: str) -> None:
        self._reset_btn.setToolTip(text)

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
        self._reset_btn.setEnabled(enabled)


class ControlPanel(QWidget):
    """User inputs and simulation execution controls."""

    paramChanged = pyqtSignal(str, float)
    softLandingChanged = pyqtSignal(bool)
    trailModeChanged = pyqtSignal(bool)
    limitCyclesToggled = pyqtSignal(bool)
    pauseToggled = pyqtSignal(bool)
    stepRequested = pyqtSignal()
    stepBackRequested = pyqtSignal()
    resetRequested = pyqtSignal()
    exportRequested = pyqtSignal()
    speedChanged = pyqtSignal(float)

    def __init__(self, params: Params, *, interactive: bool = True, headless: bool = False) -> None:
        super().__init__()
        self.params = params
        self._interactive = interactive
        self._headless = headless
        self.setMinimumWidth(280)

        root = QVBoxLayout()
        root.addWidget(self._make_param_group(params))
        root.addWidget(self._make_toggle_group(params))
        if not headless:
            # Headless rendering fixes the playback speed on the command
            # line (--speed), so the radio group would be dead weight.
            root.addWidget(self._make_speed_group())
        root.addWidget(self._make_execution_group())
        self._readout = QLabel()
        self._readout.setTextFormat(Qt.TextFormat.PlainText)
        self._readout.setStyleSheet("font-family: monospace; font-size: 11px;")
        root.addWidget(self._readout)
        root.addStretch(1)
        self.setLayout(root)

        # Replay grays the parameter widgets out; headless keeps the
        # interactive look so rendered videos show exactly what an
        # interactive user sees (values still mirror the log).
        if not interactive and not headless:
            self.set_parameters_enabled(False)

    # -- construction ---------------------------------------------------------
    def _make_param_group(self, p: Params) -> QGroupBox:
        group = QGroupBox("Control parameters")
        grid = QVBoxLayout()
        grid.setSpacing(PARAM_SPACING)
        self._sliders: dict[str, _ParamSlider] = {}
        specs: list[tuple[str, str, tuple[float, float], float, str]] = [
            ("rho", "ρ̃  (0: stand, 1: hop)", (0.0, 1.0), p.rho, ""),
            ("za", "z̃_a  target apex", ZA_RANGE, p.za, "m"),
            ("zm", "z̃_m  standing height", ZM_RANGE, p.zm, "m"),
            ("zb", "z̃_b  lower limit", ZB_RANGE, p.zb, "m"),
            ("k", "k  convergence gain", K_RANGE, p.k, ""),
            ("q_scale", "q  frequency scale", Q_SCALE_RANGE, p.q_scale, ""),
        ]
        for name, label, (lo, hi), value, unit in specs:
            slider = _ParamSlider(label, lo, hi, value, unit)
            slider.valueChanged.connect(self._make_param_handler(name))
            slider.resetClicked.connect(self._make_reset_handler(name))
            self._sliders[name] = slider
            grid.addWidget(slider)
        self._sliders["rho"].set_reset_tooltip("Toggle between stand (0) and hop (1)")
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

    def _make_reset_handler(self, name: str) -> Callable[[], None]:
        def handler() -> None:
            value = getattr(_DEFAULTS, name)
            if name == "rho":
                # The rho reset toggles between the two extremes instead.
                value = 1.0 if self.params.rho == 0.0 else 0.0
            self._make_param_handler(name)(value)

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
        self._limit_cycles = QCheckBox("Show limit-cycle orbits")
        self._limit_cycles.setChecked(True)
        self._limit_cycles.toggled.connect(self.limitCyclesToggled.emit)
        box.addWidget(self._limit_cycles)
        group.setLayout(box)
        return group

    def _make_speed_group(self) -> QGroupBox:
        group = QGroupBox("Playback speed")
        grid = QGridLayout()
        self._speed_buttons: dict[float, QRadioButton] = {}
        for i, speed in enumerate(PLAYBACK_SPEEDS):
            button = QRadioButton(f"{speed}x")
            button.setChecked(speed == 1.0)
            button.toggled.connect(self._make_speed_handler(speed))
            self._speed_buttons[speed] = button
            grid.addWidget(button, i // 2, i % 2)
        group.setLayout(grid)
        return group

    def _make_speed_handler(self, speed: float) -> Callable[[bool], None]:
        def handler(checked: bool) -> None:
            if checked:
                self.speedChanged.emit(speed)

        return handler

    def _make_execution_group(self) -> QGroupBox:
        group = QGroupBox("Execution")
        box = QVBoxLayout()

        # Transport row: icon buttons, media-player order.
        def tool_button(tooltip: str) -> QToolButton:
            btn = QToolButton()
            btn.setIconSize(QSize(20, 20))
            btn.setToolTip(tooltip)
            btn.setAccessibleName(tooltip)
            return btn

        self._step_back_btn = tool_button("Step backward (replay mode only)")
        self._step_back_btn.setIcon(_icon("mdi6.step-backward"))
        self._step_back_btn.setEnabled(False)
        self._step_back_btn.clicked.connect(self.stepBackRequested.emit)
        self._pause_btn = tool_button("Pause")
        self._pause_btn.setCheckable(True)
        self._pause_btn.toggled.connect(self._on_pause_toggled)
        self._step_btn = tool_button("Step forward")
        self._step_btn.setIcon(_icon("mdi6.step-forward"))
        self._step_btn.setEnabled(False)
        self._step_btn.clicked.connect(self.stepRequested.emit)
        self._apply_pause_visual(paused=False)
        transport = QHBoxLayout()
        transport.addStretch(1)
        for btn in (self._step_back_btn, self._pause_btn, self._step_btn):
            transport.addWidget(btn)
        transport.addStretch(1)
        box.addLayout(transport)

        # Text actions below the transport row.
        actions = QHBoxLayout()
        self._reset_btn = QPushButton("Reset")
        self._reset_btn.clicked.connect(self.resetRequested.emit)
        actions.addWidget(self._reset_btn)
        if self._interactive or self._headless:
            # Present in headless frames too, purely to mirror the
            # interactive layout (nothing can click it offscreen).
            export_btn = QPushButton("Export CSV…")
            export_btn.clicked.connect(self.exportRequested.emit)
            actions.addWidget(export_btn)
        box.addLayout(actions)
        group.setLayout(box)
        return group

    def _apply_pause_visual(self, *, paused: bool) -> None:
        """Swap the pause/play icon and tooltip with the pause state."""
        self._pause_btn.setIcon(_icon("mdi6.play" if paused else "mdi6.pause"))
        tooltip = "Resume" if paused else "Pause"
        self._pause_btn.setToolTip(tooltip)
        self._pause_btn.setAccessibleName(tooltip)

    def _update_step_buttons(self, *, paused: bool) -> None:
        self._step_btn.setEnabled(paused)
        # Stepping backward needs recorded samples ahead of the cursor,
        # so it is a replay-only affordance.
        self._step_back_btn.setEnabled(paused and not self._interactive)

    def _on_pause_toggled(self, paused: bool) -> None:
        self._apply_pause_visual(paused=paused)
        self._update_step_buttons(paused=paused)
        self.pauseToggled.emit(paused)

    def trail_mode_enabled(self) -> bool:
        return self._trail_mode.isChecked()

    def limit_cycles_enabled(self) -> bool:
        return self._limit_cycles.isChecked()

    # -- updates from the frame loop --------------------------------------------
    def set_parameters_enabled(self, enabled: bool) -> None:
        for slider in self._sliders.values():
            slider.setEnabled(enabled)
        self._soft_landing.setEnabled(enabled)

    def reflect_snapshot(self, snap: Snapshot) -> None:
        """In replay mode, mirror the logged parameter values on the widgets."""
        for name in ("rho", "za", "zm", "zb", "k", "q_scale"):
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
            self._apply_pause_visual(paused=paused)
            self._update_step_buttons(paused=paused)
            self._pause_btn.blockSignals(False)  # noqa: FBT003
