"""Phase portrait view: (z, vz) plane of the vertical COM dynamics.

Per the directive: white background; solid black axes at z = zh and
vz = 0; thin gray solution curves; dotted boundary lines at the target
apex (za) and the kinematic lower limit (zb); the current COM state as a
solid red circle; and a COM history that can be toggled between a
continuous line and a fading trail of recent states.

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

if TYPE_CHECKING:
    import numpy as np

    from rhc_demo.state import Snapshot

BACKGROUND = QColor("#ffffff")
AXIS_COLOR = QColor("#000000")
CURVE_COLOR = QColor("#b0b0b0")
BOUNDARY_COLOR = QColor("#404040")
COM_COLOR = QColor("#d62728")
HISTORY_COLOR = QColor("#e08080")
LABEL_COLOR = QColor("#606060")

# World window of the plot, in metres and metres/second.
Z_WINDOW = (0.16, 0.40)
VZ_WINDOW = (-2.0, 2.0)

MARGIN = 14.0
MIN_POLYLINE = 2  # points needed to draw a line
TRAIL_LEN = 15  # discrete-trail mode: number of recent states drawn
HISTORY_MAX = 200_000  # stored history points before compaction
HISTORY_DECIMATE = 5  # store every 5th incoming sample (1 kHz -> 200 Hz)


def _cosmetic_pen(color: QColor, width: float, style: Qt.PenStyle = Qt.PenStyle.SolidLine) -> QPen:
    pen = QPen(color, width, style)
    pen.setCosmetic(True)  # keep line width in pixels under the world transform
    return pen


class PhaseView(QWidget):
    """Phase portrait of the vertical COM dynamics."""

    def __init__(self) -> None:
        super().__init__()
        self.setMinimumSize(320, 320)
        self._curve_polys: list[QPolygonF] = []
        self._history = QPolygonF()
        self._decim_phase = 0
        self._trail: deque[QPointF] = deque(maxlen=TRAIL_LEN)
        self._snap: Snapshot | None = None
        self._trail_mode = False

    # -- data input -----------------------------------------------------------
    def set_curves(self, curves: list[tuple[np.ndarray, np.ndarray]]) -> None:
        self._curve_polys = [
            QPolygonF([QPointF(float(z[j]), float(vz[j])) for j in range(len(z))])
            for z, vz in curves
            if len(z) >= MIN_POLYLINE
        ]
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
        sx = (w - 2 * MARGIN) / (Z_WINDOW[1] - Z_WINDOW[0])
        sy = -(h - 2 * MARGIN) / (VZ_WINDOW[1] - VZ_WINDOW[0])
        return QTransform(sx, 0.0, 0.0, sy, MARGIN - Z_WINDOW[0] * sx, MARGIN - VZ_WINDOW[1] * sy)

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
        painter.drawLine(QPointF(Z_WINDOW[0], 0.0), QPointF(Z_WINDOW[1], 0.0))
        snap = self._snap
        if snap is not None and not math.isnan(snap.zh):
            painter.drawLine(QPointF(snap.zh, VZ_WINDOW[0]), QPointF(snap.zh, VZ_WINDOW[1]))

        # Dotted boundaries: target apex za and kinematic lower limit zb.
        if snap is not None:
            painter.setPen(_cosmetic_pen(BOUNDARY_COLOR, 1.0, Qt.PenStyle.DotLine))
            for value in (snap.za, snap.zb):
                if not math.isnan(value):
                    painter.drawLine(QPointF(value, VZ_WINDOW[0]), QPointF(value, VZ_WINDOW[1]))

        # Continuous history line (unless the trail mode is active).
        if not self._trail_mode and self._history.size() >= MIN_POLYLINE:
            painter.setPen(_cosmetic_pen(HISTORY_COLOR, 1.0))
            painter.drawPolyline(self._history)

    def _draw_overlay(self, painter: QPainter, tr: QTransform) -> None:
        snap = self._snap
        # Guide labels, drawn in pixel space for legible text.
        painter.setPen(QPen(LABEL_COLOR, 1))
        if snap is not None:
            for value, label, dy in ((snap.zh, "z_h", 12.0), (snap.za, "z̃_a", 24.0), (snap.zb, "z̃_b", 24.0)):
                if not math.isnan(value):
                    top = tr.map(QPointF(value, VZ_WINDOW[1]))
                    painter.drawText(QPointF(top.x() + 3.0, top.y() + dy), label)
        painter.drawText(QPointF(6.0, float(self.height()) - 6.0), "z →   (phase portrait: z vs ż)")

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
