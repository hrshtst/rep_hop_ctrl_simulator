"""Robot view: 2D schematic of the one-legged robot (paper Fig. 2).

Light gray capsule-shaped links with the knee bending to the right; the
COM drawn as a red circle with a Secchi-disk pattern whose centre marks
the model state z exactly (the hip joint hangs below it on the body, so
the joint stays visible); the ground reaction force as a blue arrow
from the contact point; the external force as a purple arrow at the
COM. The plant is a 1-DOF vertical model, so the external
force is vertical only: dragging in the view maps the vertical drag
component to fe (the horizontal component is discarded).

Contact state follows the controller phase: airborne when z > zh, in
contact during compression/extension; the leg is drawn at full reach in
flight (foot zh below the COM) and compresses, bending the knee to the
right, while z < zh in stance.
"""

from __future__ import annotations

import math
from typing import TYPE_CHECKING

from PyQt6.QtCore import QPointF, QRectF, Qt, pyqtSignal
from PyQt6.QtGui import QBrush, QColor, QPainter, QPen, QPolygonF
from PyQt6.QtWidgets import QWidget

if TYPE_CHECKING:
    from PyQt6.QtGui import QMouseEvent

    from rhc_demo.state import Snapshot

BACKGROUND = QColor("#ffffff")
LINK_COLOR = QColor("#b8b8b8")  # light gray links and joints
JOINT_COLOR = QColor("#a0a0a0")
GROUND_COLOR = QColor("#404040")
GUIDE_COLOR = QColor("#909090")
COM_RED = QColor("#d62728")
COM_WHITE = QColor("#ffffff")
GRF_COLOR = QColor("#1f6feb")  # blue: ground reaction force fz
FE_COLOR = QColor("#9467bd")  # purple: external force fe
LABEL_COLOR = QColor("#606060")

Z_VIEW_MAX = 0.60  # metres mapped to the top of the drawing area
MARGIN = 18.0

GRF_PX_PER_N = 0.5  # arrow length scaling
FE_PX_PER_N = 0.5
FE_N_PER_PX = 1.5  # drag sensitivity (larger mouse travel per newton)
FE_MAX = 800.0  # |fe| cap, N

# Link geometry in metres (scaled with the robot), radii in pixels.
BODY_LEN = 0.15  # body link above the hip: long and thin
BODY_WIDTH = 0.028
BODY_CORNER_RADIUS = 5.0  # px; near-rectangular with softened edges
LEG_WIDTH = 0.016  # thigh / shank capsule thickness
FOOT_LEN = 0.035  # simple thick-line foot segment
FOOT_WIDTH = 5.0  # px
JOINT_RADIUS = 7.0
FOOT_JOINT_RADIUS = 5.0
COM_RADIUS = 17.0
# The hip joint hangs this far below the COM, so the Secchi disk marks
# the model state z exactly while the body-thigh joint stays visible.
COM_OFFSET = COM_RADIUS + JOINT_RADIUS + 2.0

ARROW_WIDTH = 4.0
ARROW_HEAD = 15.0


def _arrow(painter: QPainter, start: QPointF, end: QPointF, color: QColor, width: float = ARROW_WIDTH) -> None:
    """Draw an arrow whose head tip lies exactly at ``end``.

    The shaft stops at the head's base: drawn through to the tip it
    would poke out of the triangle, which tapers below the stroke
    width near the tip (and the square line cap would overshoot it).
    Arrows shorter than the nominal head shrink to head-only.
    """
    dx, dy = end.x() - start.x(), end.y() - start.y()
    length = math.hypot(dx, dy)
    if length < 1.0:
        return
    ux, uy = dx / length, dy / length
    head = min(ARROW_HEAD, length)
    base = QPointF(end.x() - head * ux, end.y() - head * uy)
    left = QPointF(base.x() + head * 0.5 * uy, base.y() - head * 0.5 * ux)
    right = QPointF(base.x() - head * 0.5 * uy, base.y() + head * 0.5 * ux)
    if length > head:
        painter.setPen(QPen(color, width))
        painter.drawLine(start, base)
    painter.setPen(Qt.PenStyle.NoPen)
    painter.setBrush(QBrush(color))
    painter.drawPolygon(QPolygonF([end, left, right]))


def _link(painter: QPainter, a: QPointF, b: QPointF, width: float, radius: float | None = None) -> None:
    """Draw a rounded-rectangle link outline spanning joints ``a`` to ``b``.

    Corners default to a capsule (radius = half the width); pass a
    smaller ``radius`` for a near-rectangular link.
    """
    dx, dy = b.x() - a.x(), b.y() - a.y()
    length = math.hypot(dx, dy)
    if length < 1.0:
        return
    half = width / 2.0
    if radius is None:
        radius = half
    painter.save()
    painter.translate(a)
    painter.rotate(math.degrees(math.atan2(dy, dx)))
    painter.drawRoundedRect(QRectF(-half, -half, length + width, width), radius, radius)
    painter.restore()


class RobotView(QWidget):
    """Schematic one-legged robot, driven by snapshots."""

    feChanged = pyqtSignal(float)  # emitted while dragging; 0.0 on release

    def __init__(self, *, interactive: bool = True) -> None:
        super().__init__()
        self.setMinimumSize(280, 320)
        self._interactive = interactive
        self._snap: Snapshot | None = None
        self._drag_origin: QPointF | None = None
        if interactive:
            self.setCursor(Qt.CursorShape.OpenHandCursor)

    def set_snapshot(self, snap: Snapshot) -> None:
        self._snap = snap
        self.update()

    # -- geometry helpers ----------------------------------------------------
    def _scale(self) -> float:
        return (self.height() - 2 * MARGIN) / Z_VIEW_MAX

    def _ground_y(self) -> float:
        return float(self.height()) - MARGIN

    def _y(self, z: float) -> float:
        return self._ground_y() - z * self._scale()

    def _com_x(self) -> float:
        # Keep the leg column left of centre so force arrows have room.
        return 0.42 * float(self.width())

    # -- external force input (vertical drag) -----------------------------------
    def mousePressEvent(self, event: QMouseEvent) -> None:
        if self._interactive and event.button() == Qt.MouseButton.LeftButton:
            self._drag_origin = event.position()
            self.setCursor(Qt.CursorShape.ClosedHandCursor)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        if self._drag_origin is not None:
            # Upward drag (negative pixel dy) -> positive (upward) force.
            dy = self._drag_origin.y() - event.position().y()
            force = max(-FE_MAX, min(FE_MAX, dy * FE_N_PER_PX))
            self.feChanged.emit(force)

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        del event
        if self._drag_origin is not None:
            self._drag_origin = None
            self.setCursor(Qt.CursorShape.OpenHandCursor)
            self.feChanged.emit(0.0)

    # -- painting ---------------------------------------------------------------
    def paintEvent(self, event) -> None:
        del event
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.fillRect(self.rect(), BACKGROUND)
        self._draw_ground_and_guides(painter)
        snap = self._snap
        if snap is None or math.isnan(snap.z):
            return
        self._draw_robot(painter, snap)
        self._draw_forces(painter, snap)

    def _draw_ground_and_guides(self, painter: QPainter) -> None:
        w = float(self.width())
        gy = self._ground_y()
        painter.setPen(QPen(GROUND_COLOR, 2))
        painter.drawLine(QPointF(MARGIN, gy), QPointF(w - MARGIN, gy))
        # Ground hatching.
        painter.setPen(QPen(GROUND_COLOR, 1))
        x = MARGIN
        while x < w - MARGIN:
            painter.drawLine(QPointF(x, gy), QPointF(x - 6.0, gy + 7.0))
            x += 12.0
        snap = self._snap
        if snap is None:
            return
        painter.setPen(QPen(LABEL_COLOR, 1))
        # z_h sits only 5 mm above z̃_m, so its label moves further left
        # to keep the two from overlapping.
        for value, style, label, dx in (
            (snap.za, Qt.PenStyle.DashLine, "z̃_a", 24.0),
            (snap.zh, Qt.PenStyle.SolidLine, "z_h", 64.0),
            (snap.zm, Qt.PenStyle.DotLine, "z̃_m", 24.0),
            (snap.zb, Qt.PenStyle.DashLine, "z̃_b", 24.0),
        ):
            if math.isnan(value):
                continue
            y = self._y(value)
            painter.setPen(QPen(GUIDE_COLOR, 1, style))
            painter.drawLine(QPointF(MARGIN, y), QPointF(w - MARGIN, y))
            painter.setPen(QPen(LABEL_COLOR, 1))
            painter.drawText(QPointF(w - MARGIN - dx, y - 3.0), label)

    def _leg_points(self, snap: Snapshot) -> tuple[QPointF, QPointF, QPointF]:
        """Hip, knee and foot positions in pixels; knee bends right.

        The model state z is the COM height; the hip joint hangs
        COM_OFFSET below it on the body, so the Secchi disk drawn at
        the COM stays clear of the joint. The foot is anchored from
        the COM (reach to the ground in stance, frozen at full reach
        in flight), leaving its position independent of the offset.
        """
        cx = self._com_x()
        zh = snap.zh if not math.isnan(snap.zh) else 0.26
        scale = self._scale()
        com_y = self._y(snap.z)
        hip = QPointF(cx, com_y + COM_OFFSET)
        reach = min(snap.z, zh) if snap.contact or snap.z <= zh else zh
        foot = QPointF(cx, com_y + reach * scale)
        # Two equal links, slightly longer than half the hip-to-foot
        # span at full reach so the knee keeps a visible bend;
        # perpendicular offset points right.
        link = 0.505 * max(zh * scale - COM_OFFSET, 1.0)
        d = foot.y() - hip.y()
        half = 0.5 * d
        offset = math.sqrt(max(link * link - half * half, 0.0))
        knee = QPointF(cx + offset, hip.y() + half)
        return hip, knee, foot

    def _com_center(self, hip: QPointF) -> QPointF:
        """Centre of the Secchi disk: exactly the COM height z."""
        return QPointF(hip.x(), hip.y() - COM_OFFSET)

    def _draw_robot(self, painter: QPainter, snap: Snapshot) -> None:
        hip, knee, foot = self._leg_points(snap)
        scale = self._scale()

        # Links: the long thin near-rectangular body above the hip and
        # capsule-shaped thigh and shank; the foot stays a simple thick
        # line segment.
        painter.setPen(QPen(LINK_COLOR, 2))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        _link(painter, hip, QPointF(hip.x(), hip.y() - BODY_LEN * scale), BODY_WIDTH * scale, BODY_CORNER_RADIUS)
        _link(painter, hip, knee, LEG_WIDTH * scale)
        _link(painter, knee, foot, LEG_WIDTH * scale)
        painter.setPen(QPen(LINK_COLOR, FOOT_WIDTH, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap))
        painter.drawLine(foot, QPointF(foot.x() + FOOT_LEN * scale, foot.y()))

        # Joints; the foot joint is smaller to match its plain link.
        painter.setPen(QPen(JOINT_COLOR, 1.5))
        painter.setBrush(QBrush(BACKGROUND))
        for joint in (hip, knee):
            painter.drawEllipse(joint, JOINT_RADIUS, JOINT_RADIUS)
        painter.drawEllipse(foot, FOOT_JOINT_RADIUS, FOOT_JOINT_RADIUS)

        # COM: red circle with a Secchi-disk pattern (alternating
        # quadrants), centred exactly on the model state z.
        com = self._com_center(hip)
        painter.setPen(QPen(COM_RED.darker(130), 1.5))
        painter.setBrush(QBrush(COM_WHITE))
        painter.drawEllipse(com, COM_RADIUS, COM_RADIUS)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(COM_RED))
        for start_angle in (0, 180):  # degrees; Qt uses 1/16 deg units
            painter.drawPie(
                int(com.x() - COM_RADIUS),
                int(com.y() - COM_RADIUS),
                int(2 * COM_RADIUS),
                int(2 * COM_RADIUS),
                start_angle * 16,
                90 * 16,
            )
        painter.setPen(QPen(COM_RED.darker(130), 1.5))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawEllipse(com, COM_RADIUS, COM_RADIUS)

    def _draw_forces(self, painter: QPainter, snap: Snapshot) -> None:
        hip, _, foot = self._leg_points(snap)
        # Ground reaction force: blue arrow from the contact point (stance
        # only). The magnitude label stays put near the ground instead of
        # riding the arrow tip.
        if snap.contact and snap.fz > 1.0:
            length = min(snap.fz * GRF_PX_PER_N, foot.y() - MARGIN)
            end = QPointF(foot.x(), foot.y() - length)
            _arrow(painter, foot, end, GRF_COLOR)
            painter.setPen(QPen(GRF_COLOR, 1))
            painter.drawText(QPointF(foot.x() + 16.0, self._ground_y() - 8.0), f"f_z = {snap.fz:.0f} N")
        # External force: purple vertical arrow at the COM, labelled next
        # to the disk so the value is readable regardless of arrow length.
        if abs(snap.fe) > 1.0:
            com = self._com_center(hip)
            end = QPointF(com.x(), com.y() - snap.fe * FE_PX_PER_N)
            _arrow(painter, com, end, FE_COLOR)
            painter.setPen(QPen(FE_COLOR, 1))
            painter.drawText(QPointF(com.x() + COM_RADIUS + 8.0, com.y() + 4.0), f"f_e = {snap.fe:+.0f} N")
