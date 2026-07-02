"""Scrolling time series of COM height and GRF over the last few seconds.

Mirrors the paper's time-series figures (z on top, fz below) so landing
impacts — and their absence under the soft-landing strategy — are visible
live, with a peak-hold label for the windowed maximum GRF. The static frame
(axes, guides) is cached and the traces are blitted each frame, like
:class:`rhc_demo.phase_widget.PhaseWidget`.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from PyQt6.QtWidgets import QVBoxLayout, QWidget

if TYPE_CHECKING:
    import numpy as np

    from rhc_demo.driver import Params

GRAVITY = 9.80665
WINDOW_S = 3.0  # seconds of history shown
Z_LIM = (0.18, 0.40)
FZ_LIM = (0.0, 1100.0)
Z_COLOR = "#1f6feb"
FZ_COLOR = "#1b9e3e"
PEAK_COLOR = "#e6091c"
APEX_COLOR = "#e6091c"
GROUND_COLOR = "#555555"


class TimeSeriesWidget(QWidget):
    """Two stacked strip charts of z(t) and fz(t) with a peak-hold GRF label."""

    def __init__(self) -> None:
        super().__init__()
        self.fig = Figure(figsize=(6.0, 2.6), layout="constrained")
        self.canvas = FigureCanvas(self.fig)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.canvas)
        self.ax_z, self.ax_fz = self.fig.subplots(2, 1, sharex=True)
        self._bg = None
        self._z_line = None
        self._fz_line = None
        self._peak_text = None
        self._init_axes()

    def _init_axes(self) -> None:
        self.ax_z.set_ylim(Z_LIM)
        self.ax_z.set_ylabel("z [m]")
        self.ax_fz.set_ylim(FZ_LIM)
        self.ax_fz.set_ylabel("fz [N]")
        self.ax_fz.set_xlabel("time [s]")
        for ax in (self.ax_z, self.ax_fz):
            ax.set_xlim(-WINDOW_S, 0.0)
            ax.grid(visible=True, lw=0.3, alpha=0.4)
        self._make_artists()

    def _make_artists(self) -> None:
        (self._z_line,) = self.ax_z.plot([], [], lw=1.2, color=Z_COLOR, animated=True)
        (self._fz_line,) = self.ax_fz.plot([], [], lw=1.2, color=FZ_COLOR, animated=True)
        self._peak_text = self.ax_fz.text(
            0.99,
            0.92,
            "",
            transform=self.ax_fz.transAxes,
            ha="right",
            va="top",
            color=PEAK_COLOR,
            fontsize=9,
            animated=True,
        )

    def set_guides(self, params: Params) -> None:
        """Redraw the static frame (axes and guide lines) and cache it."""
        for ax in (self.ax_z, self.ax_fz):
            ax.clear()
        self._init_axes()
        self.ax_z.axhline(params.za, ls="--", lw=0.8, color=APEX_COLOR)
        self.ax_z.axhline(params.zh, ls=":", lw=0.8, color=GROUND_COLOR)
        self.ax_fz.axhline(params.mass * GRAVITY, ls=":", lw=0.8, color=GROUND_COLOR)  # weight mg
        self.canvas.draw()
        self._bg = self.canvas.copy_from_bbox(self.fig.bbox)

    def update_live(self, t_ago: np.ndarray, z: np.ndarray, fz: np.ndarray) -> None:
        """Blit the traces and the windowed peak-GRF label."""
        if self._bg is None:
            self.canvas.draw()
            self._bg = self.canvas.copy_from_bbox(self.fig.bbox)
        self.canvas.restore_region(self._bg)
        self._z_line.set_data(t_ago, z)
        self._fz_line.set_data(t_ago, fz)
        if len(fz) > 0:
            self._peak_text.set_text(f"peak fz: {float(fz.max()):.0f} N")
        self.ax_z.draw_artist(self._z_line)
        self.ax_fz.draw_artist(self._fz_line)
        self.ax_fz.draw_artist(self._peak_text)
        self.canvas.blit(self.fig.bbox)
