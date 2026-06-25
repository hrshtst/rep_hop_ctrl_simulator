"""Phase-portrait widget: a cached (z, vz) trajectory family with a live trail.

The static field (the paper-style family of solution curves plus the switching
surface, apex/crouch bounds and equilibrium marker) is drawn once per parameter
change and cached as a background; the live robot trail is blitted on top each
frame so animation stays cheap.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from PyQt6.QtWidgets import QVBoxLayout, QWidget

if TYPE_CHECKING:
    import numpy as np

    from rhc_demo.driver import Params

TRAIL_COLOR = "#1f6feb"
POINT_COLOR = "#e6091c"
XLIM = (0.16, 0.36)
YLIM = (-1.6, 1.6)


class PhaseWidget(QWidget):
    """Embeds a matplotlib canvas showing the (z, vz) phase portrait."""

    def __init__(self) -> None:
        super().__init__()
        self.fig = Figure(figsize=(4.0, 3.2), layout="constrained")
        self.canvas = FigureCanvas(self.fig)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.canvas)
        self.ax = self.fig.add_subplot(111)
        self._bg = None
        self._trail = None
        self._point = None
        self._init_axes()

    def _init_axes(self) -> None:
        ax = self.ax
        ax.set_xlabel("position [m]")
        ax.set_ylabel("velocity [m/s]")
        ax.set_xlim(XLIM)
        ax.set_ylim(YLIM)
        ax.grid(visible=True, lw=0.3, alpha=0.4)
        (self._trail,) = ax.plot([], [], lw=1.4, color=TRAIL_COLOR, animated=True, zorder=5)
        (self._point,) = ax.plot([], [], "o", color=POINT_COLOR, ms=6, animated=True, zorder=6)

    def set_field(self, trajectories: list[tuple[np.ndarray, np.ndarray]], params: Params) -> None:
        """Redraw the static phase-portrait background and cache it."""
        ax = self.ax
        ax.clear()
        for z, vz in trajectories:
            ax.plot(z, vz, lw=0.3, color="black", alpha=0.55)
        ax.axvline(params.zh, lw=0.8, color="black")  # switching surface
        ax.axhline(0.0, lw=0.8, color="black")
        if params.rho > 0.0:
            ax.axvline(params.za, ls="--", lw=1.0, color="black")
            ax.axvline(params.zb, ls="--", lw=1.0, color="black")
        else:
            ax.plot(params.zm, 0.0, "o", color="black", ms=4)  # equilibrium
        ax.set_xlabel("position [m]")
        ax.set_ylabel("velocity [m/s]")
        ax.set_xlim(XLIM)
        ax.set_ylim(YLIM)
        ax.grid(visible=True, lw=0.3, alpha=0.4)
        (self._trail,) = ax.plot([], [], lw=1.4, color=TRAIL_COLOR, animated=True, zorder=5)
        (self._point,) = ax.plot([], [], "o", color=POINT_COLOR, ms=6, animated=True, zorder=6)
        self.canvas.draw()
        self._bg = self.canvas.copy_from_bbox(ax.bbox)

    def update_live(self, trail_z: np.ndarray, trail_vz: np.ndarray) -> None:
        """Blit the live trail and current point over the cached background."""
        if self._bg is None:
            self.canvas.draw()
            self._bg = self.canvas.copy_from_bbox(self.ax.bbox)
        self.canvas.restore_region(self._bg)
        self._trail.set_data(trail_z, trail_vz)
        if len(trail_z) > 0:
            self._point.set_data([trail_z[-1]], [trail_vz[-1]])
        self.ax.draw_artist(self._trail)
        self.ax.draw_artist(self._point)
        self.canvas.blit(self.ax.bbox)
