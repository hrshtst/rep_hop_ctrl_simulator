"""Phase portrait view: (z, vz) plane of the vertical COM dynamics.

Per the directive: white background; solid black axes at z = zh and
vz = 0; thin gray solution curves; the limit-cycle orbits drawn over
them (bottom to top: the soft-landing cushion ellipse in dotted green,
the full stance ellipse in dotted blue, the true limit cycle in solid
blue), toggleable from the control panel; dotted boundary lines at the
target apex (za) and the kinematic lower limit (zb); the current COM
state as a solid red circle; and a COM history that can be toggled
between a continuous line and a fading trail of recent states.

For debugging the curve family, the initial seeds of the solution
curves can be overlaid as black circles (``show_seeds``; enabled with
the ``--show-seeds`` command-line option).

Geometry is kept in world coordinates (QPolygonF) and mapped with a
QTransform at paint time, so the history polyline is append-only — no
per-frame rebuilds no matter how long the session runs.
"""

from __future__ import annotations

import math
from collections import deque
from typing import TYPE_CHECKING

from PyQt6.QtCore import QPointF, Qt
from PyQt6.QtGui import QBrush, QColor, QPainter, QPen, QPolygonF, QTransform
from PyQt6.QtWidgets import QWidget

from rhc_demo.params import PHASE_VZ_RANGE, PHASE_Z_RANGE

if TYPE_CHECKING:
    import numpy as np

    from rhc_demo.state import Snapshot

BACKGROUND = QColor("#ffffff")
AXIS_COLOR = QColor("#000000")
CURVE_COLOR = QColor("#b0b0b0")
CYCLE_COLOR = QColor("#1f77b4")  # blue: true limit cycle / stance ellipse
CUSHION_COLOR = QColor("#2ca02c")  # green: soft-landing cushion ellipse
BOUNDARY_COLOR = QColor("#404040")
COM_COLOR = QColor("#d62728")
HISTORY_COLOR = QColor("#e08080")
LABEL_COLOR = QColor("#606060")
SEED_COLOR = QColor("#000000")  # debug markers for curve seeds
SEED_RADIUS = 3.0

MARGIN = 14.0
MIN_POLYLINE = 2  # points needed to draw a line
TRAIL_LEN = 15  # discrete-trail mode: number of recent states drawn
HISTORY_MAX = 200_000  # stored history points before compaction
HISTORY_DECIMATE = 5  # store every 5th incoming sample (1 kHz -> 200 Hz)
CYCLE_WIDTH = 2.0  # px; shared by all three limit-cycle orbits


def _cosmetic_pen(color: QColor, width: float, style: Qt.PenStyle = Qt.PenStyle.SolidLine) -> QPen:
    pen = QPen(color, width, style)
    pen.setCosmetic(True)  # keep line width in pixels under the world transform
    return pen


def _polygon(curve: tuple[np.ndarray, np.ndarray] | None) -> QPolygonF | None:
    if curve is None:
        return None
    z, vz = curve
    return QPolygonF([QPointF(float(z[j]), float(vz[j])) for j in range(len(z))])


class PhaseView(QWidget):
    """Phase portrait of the vertical COM dynamics."""

    def __init__(self, *, show_seeds: bool = False) -> None:
        super().__init__()
        self.setMinimumSize(400, 320)
        self._curve_polys: list[QPolygonF] = []
        self._cycle_poly: QPolygonF | None = None
        self._ellipse_poly: QPolygonF | None = None
        self._cushion_poly: QPolygonF | None = None
        self._show_cycles = True
        self._seed_pts: list[QPointF] = []
        self._show_seeds = show_seeds
        self._history = QPolygonF()
        self._decim_phase = 0
        self._trail: deque[QPointF] = deque(maxlen=TRAIL_LEN)
        self._snap: Snapshot | None = None
        # Default to the fading trail: the continuous full-history line is
        # easily mistaken for a solution curve. The panel toggle switches.
        self._trail_mode = True

    # -- data input -----------------------------------------------------------
    def set_curves(
        self,
        curves: list[tuple[np.ndarray, np.ndarray]],
        seeds: list[tuple[float, float]] | None = None,
    ) -> None:
        self._curve_polys = [
            QPolygonF([QPointF(float(z[j]), float(vz[j])) for j in range(len(z))])
            for z, vz in curves
            if len(z) >= MIN_POLYLINE
        ]
        self._seed_pts = [QPointF(z0, vz0) for z0, vz0 in seeds] if seeds else []
        self.update()

    def set_limit_cycle(
        self,
        cycle: tuple[np.ndarray, np.ndarray] | None,
        ellipse: tuple[np.ndarray, np.ndarray] | None,
    ) -> None:
        """Set the true limit cycle and its full stance ellipse (None: no cycle)."""
        self._cycle_poly = _polygon(cycle)
        self._ellipse_poly = _polygon(ellipse)
        self.update()

    def set_cushion_ellipse(self, ellipse: tuple[np.ndarray, np.ndarray] | None) -> None:
        """Set the transient soft-landing cushion ellipse, or None to hide it."""
        self._cushion_poly = _polygon(ellipse)
        self.update()

    def set_show_cycles(self, on: bool) -> None:
        """Toggle visibility of all limit-cycle orbit overlays."""
        self._show_cycles = on
        self.update()

    def set_show_seeds(self, on: bool) -> None:
        """Toggle the debug overlay of curve seeds as black circles."""
        self._show_seeds = on
        self.update()

    def push_samples(self, chunks: list[tuple[np.ndarray, np.ndarray]]) -> None:
        for z, vz in chunks:
            for j in range(len(z)):
                if self._decim_phase == 0:
                    self._history.append(QPointF(float(z[j]), float(vz[j])))
                self._decim_phase = (self._decim_phase + 1) % HISTORY_DECIMATE
        if self._history.size() > HISTORY_MAX:
            self._history = QPolygonF([self._history.at(i) for i in range(0, self._history.size(), 2)])

    def set_snapshot(self, snap: Snapshot) -> None:
        self._snap = snap
        if not math.isnan(snap.z):
            self._trail.append(QPointF(snap.z, snap.vz))
        self.update()

    def clear_history(self) -> None:
        self._history = QPolygonF()
        self._trail.clear()
        self._decim_phase = 0
        self.update()

    def set_trail_mode(self, on: bool) -> None:
        self._trail_mode = on
        self.update()

    # -- painting -----------------------------------------------------------------
    def _world_transform(self) -> QTransform:
        w, h = float(self.width()), float(self.height())
        sx = (w - 2 * MARGIN) / (PHASE_Z_RANGE[1] - PHASE_Z_RANGE[0])
        sy = -(h - 2 * MARGIN) / (PHASE_VZ_RANGE[1] - PHASE_VZ_RANGE[0])
        return QTransform(sx, 0.0, 0.0, sy, MARGIN - PHASE_Z_RANGE[0] * sx, MARGIN - PHASE_VZ_RANGE[1] * sy)

    def paintEvent(self, event) -> None:
        del event
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.fillRect(self.rect(), BACKGROUND)
        tr = self._world_transform()

        painter.setTransform(tr)
        self._draw_world(painter)
        painter.resetTransform()
        self._draw_overlay(painter, tr)

    def _draw_world(self, painter: QPainter) -> None:
        # Solution curves.
        painter.setPen(_cosmetic_pen(CURVE_COLOR, 1.0))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        for poly in self._curve_polys:
            painter.drawPolyline(poly)

        # Solid black axes: vz = 0 and z = zh.
        painter.setPen(_cosmetic_pen(AXIS_COLOR, 1.0))
        painter.drawLine(QPointF(PHASE_Z_RANGE[0], 0.0), QPointF(PHASE_Z_RANGE[1], 0.0))
        snap = self._snap
        if snap is not None and not math.isnan(snap.zh):
            painter.drawLine(QPointF(snap.zh, PHASE_VZ_RANGE[0]), QPointF(snap.zh, PHASE_VZ_RANGE[1]))

        # Dotted boundaries: target apex za and kinematic lower limit zb.
        if snap is not None:
            painter.setPen(_cosmetic_pen(BOUNDARY_COLOR, 1.0, Qt.PenStyle.DotLine))
            for value in (snap.za, snap.zb):
                if not math.isnan(value):
                    painter.drawLine(QPointF(value, PHASE_VZ_RANGE[0]), QPointF(value, PHASE_VZ_RANGE[1]))

        # Continuous history line (unless the trail mode is active).
        if not self._trail_mode and self._history.size() >= MIN_POLYLINE:
            painter.setPen(_cosmetic_pen(HISTORY_COLOR, 1.0))
            painter.drawPolyline(self._history)

        # Limit-cycle orbits, drawn after the solution curves and guides so
        # they are never obscured. Bottom to top: the transient soft-landing
        # cushion ellipse (dotted green), the full stance ellipse without
        # the lift-off cut-off (dotted blue), the true limit cycle (solid
        # blue).
        if self._show_cycles:
            painter.setBrush(Qt.BrushStyle.NoBrush)
            for poly, pen in (
                (self._cushion_poly, _cosmetic_pen(CUSHION_COLOR, CYCLE_WIDTH, Qt.PenStyle.DotLine)),
                (self._ellipse_poly, _cosmetic_pen(CYCLE_COLOR, CYCLE_WIDTH, Qt.PenStyle.DotLine)),
                (self._cycle_poly, _cosmetic_pen(CYCLE_COLOR, CYCLE_WIDTH)),
            ):
                if poly is not None and poly.size() >= MIN_POLYLINE:
                    painter.setPen(pen)
                    painter.drawPolyline(poly)

    def _draw_overlay(self, painter: QPainter, tr: QTransform) -> None:
        snap = self._snap
        # Guide labels, drawn in pixel space for legible text.
        painter.setPen(QPen(LABEL_COLOR, 1))
        if snap is not None:
            for value, label, dy in ((snap.zh, "z_h", 12.0), (snap.za, "z̃_a", 24.0), (snap.zb, "z̃_b", 24.0)):
                if not math.isnan(value):
                    top = tr.map(QPointF(value, PHASE_VZ_RANGE[1]))
                    painter.drawText(QPointF(top.x() + 3.0, top.y() + dy), label)
        painter.drawText(QPointF(6.0, float(self.height()) - 6.0), "z →   (phase portrait: z vs ż)")

        # Debug overlay: initial seeds of the solution curves.
        if self._show_seeds and self._seed_pts:
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(QBrush(SEED_COLOR))
            for seed in self._seed_pts:
                painter.drawEllipse(tr.map(seed), SEED_RADIUS, SEED_RADIUS)

        # Fading discrete trail of recent states.
        if self._trail_mode and self._trail:
            painter.setPen(Qt.PenStyle.NoPen)
            take = len(self._trail)
            for i, world_pt in enumerate(self._trail):
                age = 1.0 - (i + 1) / take  # 0 = newest, -> 1 = oldest
                color = QColor(COM_COLOR)
                color.setRedF(color.redF() + (1.0 - color.redF()) * age)
                color.setGreenF(color.greenF() + (1.0 - color.greenF()) * age)
                color.setBlueF(color.blueF() + (1.0 - color.blueF()) * age)
                painter.setBrush(QBrush(color))
                painter.drawEllipse(tr.map(world_pt), 3.5, 3.5)

        # Current COM state: solid red circle.
        if snap is not None and not math.isnan(snap.z):
            painter.setPen(QPen(COM_COLOR.darker(120), 1))
            painter.setBrush(QBrush(COM_COLOR))
            painter.drawEllipse(tr.map(QPointF(snap.z, snap.vz)), 5.0, 5.0)
