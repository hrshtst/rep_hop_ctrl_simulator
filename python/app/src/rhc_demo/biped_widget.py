"""2D biped animation: COM height and contact state, drawn with QPainter."""

from __future__ import annotations

from typing import TYPE_CHECKING

from PyQt6.QtCore import QPointF, Qt
from PyQt6.QtGui import QBrush, QColor, QPainter, QPen
from PyQt6.QtWidgets import QWidget

if TYPE_CHECKING:
    from PyQt6.QtGui import QPaintEvent

CONTACT_COLOR = QColor("#1b9e3e")  # green when on the ground
FLIGHT_COLOR = QColor("#1f6feb")  # blue when airborne
APEX_COLOR = QColor("#e6091c")
GROUND_COLOR = QColor("#555555")

Z_VIEW_MAX = 0.42  # metres mapped to the top of the widget
FOOT_SPREAD = 0.05  # horizontal half-distance between feet, in metres


class BipedWidget(QWidget):
    """Draws a point-mass body on two legs; colour shows stance vs flight."""

    def __init__(self) -> None:
        super().__init__()
        self.setMinimumSize(220, 320)
        self._z = 0.255
        self._contact = True
        self._zh = 0.26
        self._za = 0.28

    def set_state(self, z: float, *, contact: bool, zh: float, za: float) -> None:
        self._z = z
        self._contact = contact
        self._zh = zh
        self._za = za
        self.update()

    def _y(self, z: float, ground_y: float, scale: float) -> float:
        return ground_y - z * scale

    def paintEvent(self, event: QPaintEvent) -> None:
        del event
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        w = self.width()
        h = self.height()
        painter.fillRect(0, 0, w, h, QColor("#fbfbfb"))

        margin = 24.0
        ground_y = h - margin
        scale = (ground_y - margin) / Z_VIEW_MAX
        cx = w / 2.0
        body_color = CONTACT_COLOR if self._contact else FLIGHT_COLOR

        # Ground.
        painter.setPen(QPen(GROUND_COLOR, 2))
        painter.drawLine(int(margin), int(ground_y), int(w - margin), int(ground_y))

        # Target apex height (dashed red) and lift-off height (dotted grey).
        guides = (
            (self._za, APEX_COLOR, Qt.PenStyle.DashLine),
            (self._zh, GROUND_COLOR, Qt.PenStyle.DotLine),
        )
        for value, color, style in guides:
            y = self._y(value, ground_y, scale)
            painter.setPen(QPen(color, 1, style))
            painter.drawLine(int(margin), int(y), int(w - margin), int(y))

        body_y = self._y(self._z, ground_y, scale)

        # Feet: on the ground during stance, retracted below the body in flight.
        foot_y = ground_y if self._contact else body_y + 0.06 * scale
        painter.setPen(QPen(body_color, 3))
        for sign in (-1.0, 1.0):
            foot_x = cx + sign * FOOT_SPREAD * scale
            painter.drawLine(QPointF(cx, body_y), QPointF(foot_x, foot_y))
            painter.setBrush(QBrush(body_color))
            painter.drawEllipse(QPointF(foot_x, foot_y), 4, 4)

        # Body (COM).
        painter.setBrush(QBrush(body_color))
        painter.setPen(QPen(body_color.darker(120), 2))
        painter.drawEllipse(QPointF(cx, body_y), 14, 14)
