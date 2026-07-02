"""Slider panel for live control parameters, plus disturbance/reset buttons."""

from __future__ import annotations

from typing import TYPE_CHECKING

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QCheckBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSlider,
    QVBoxLayout,
    QWidget,
)

if TYPE_CHECKING:
    from rhc_demo.driver import Params


class LabeledSlider(QWidget):
    """A float-valued slider with a name and a live value readout."""

    valueChanged = pyqtSignal(float)

    def __init__(self, label: str, vmin: float, vmax: float, value: float, step: float, fmt: str = "{:.3f}") -> None:
        super().__init__()
        self._vmin = vmin
        self._step = step
        self._fmt = fmt
        self._n = round((vmax - vmin) / step)
        name = QLabel(label)
        name.setMinimumWidth(34)
        self._slider = QSlider(Qt.Orientation.Horizontal)
        self._slider.setRange(0, self._n)
        self._slider.setValue(self._to_int(value))
        self._readout = QLabel(fmt.format(value))
        self._readout.setMinimumWidth(54)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(name)
        layout.addWidget(self._slider)
        layout.addWidget(self._readout)
        self._slider.valueChanged.connect(self._on_change)

    def _to_int(self, v: float) -> int:
        return round((v - self._vmin) / self._step)

    def _to_float(self, i: int) -> float:
        return self._vmin + i * self._step

    def _on_change(self, i: int) -> None:
        v = self._to_float(i)
        self._readout.setText(self._fmt.format(v))
        self.valueChanged.emit(v)


class ControlsPanel(QWidget):
    """Bundles the parameter sliders and the disturbance/reset buttons."""

    rhoChanged = pyqtSignal(float)
    zaChanged = pyqtSignal(float)
    zmChanged = pyqtSignal(float)
    zbChanged = pyqtSignal(float)
    zhChanged = pyqtSignal(float)
    kChanged = pyqtSignal(float)
    massChanged = pyqtSignal(float)
    softLandingChanged = pyqtSignal(bool)
    disturbUpRequested = pyqtSignal()
    disturbDownRequested = pyqtSignal()
    resetRequested = pyqtSignal()

    def __init__(self, params: Params) -> None:
        super().__init__()
        self.setMaximumWidth(320)
        layout = QVBoxLayout(self)

        layout.addWidget(QLabel("<b>Mode</b>"))
        rho = LabeledSlider("ρ", 0.0, 1.0, params.rho, 0.01, "{:.2f}")
        rho.valueChanged.connect(self.rhoChanged)
        layout.addWidget(rho)

        layout.addWidget(QLabel("<b>Target heights [m]</b>"))
        for label, sig, lo, hi, val in (
            ("z̃a", self.zaChanged, 0.20, 0.36, params.za),
            ("z̃m", self.zmChanged, 0.20, 0.30, params.zm),
            ("z̃b", self.zbChanged, 0.18, 0.27, params.zb),
            ("zh", self.zhChanged, 0.22, 0.30, params.zh),
        ):
            slider = LabeledSlider(label, lo, hi, val, 0.001)
            slider.valueChanged.connect(sig)
            layout.addWidget(slider)

        layout.addWidget(QLabel("<b>Dynamics</b>"))
        k = LabeledSlider("k", 0.5, 10.0, params.k, 0.1, "{:.1f}")
        k.valueChanged.connect(self.kChanged)
        layout.addWidget(k)
        mass = LabeledSlider("m", 1.0, 30.0, params.mass, 0.5, "{:.1f}")
        mass.valueChanged.connect(self.massChanged)
        layout.addWidget(mass)

        layout.addWidget(QLabel("<b>Landing</b>"))
        soft_landing = QCheckBox("Soft landing")
        soft_landing.setChecked(params.soft_landing)
        soft_landing.toggled.connect(self.softLandingChanged)
        layout.addWidget(soft_landing)

        buttons = QHBoxLayout()
        disturb_up_btn = QPushButton("Disturb ↑")
        disturb_up_btn.clicked.connect(lambda: self.disturbUpRequested.emit())
        disturb_down_btn = QPushButton("Disturb ↓")
        disturb_down_btn.clicked.connect(lambda: self.disturbDownRequested.emit())
        reset_btn = QPushButton("Reset")
        reset_btn.clicked.connect(lambda: self.resetRequested.emit())
        buttons.addWidget(disturb_up_btn)
        buttons.addWidget(disturb_down_btn)
        buttons.addWidget(reset_btn)
        layout.addLayout(buttons)

        self._readout = QLabel("")
        self._readout.setWordWrap(True)
        layout.addWidget(self._readout)
        layout.addStretch(1)

    def set_readout(self, text: str) -> None:
        self._readout.setText(text)
