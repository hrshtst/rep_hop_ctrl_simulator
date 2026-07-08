"""The per-frame snapshot shared between simulation sources and the UI.

Both the live engine and the CSV replay source produce these, so the
window and widgets are agnostic to where the data comes from.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

PHASE_NAMES = {-1: "invalid", 0: "falling", 1: "compression", 2: "extension", 3: "rising"}
CONTACT_PHASES = frozenset({1, 2})  # compression, extension


@dataclass(frozen=True)
class Snapshot:
    """Everything the widgets need to draw one frame."""

    t: float = 0.0
    z: float = float("nan")
    vz: float = 0.0
    fz: float = 0.0
    fe: float = 0.0
    phase: int = -1
    hops: int = 0
    za: float = float("nan")
    zh: float = float("nan")
    zm: float = float("nan")
    zb: float = float("nan")
    rho: float = float("nan")
    k: float = float("nan")
    q_scale: float = float("nan")
    soft_landing: bool = True
    p_za: float = float("nan")
    p_zm: float = float("nan")
    p_zb: float = float("nan")
    p_rho: float = float("nan")
    playing: bool = False
    finished: bool = False

    @property
    def contact(self) -> bool:
        return self.phase in CONTACT_PHASES

    @property
    def phase_name(self) -> str:
        return PHASE_NAMES.get(self.phase, "invalid")


def fmt(value: float, digits: int = 3) -> str:
    """Format a float for the readout; NaN renders as an em dash."""
    return "—" if math.isnan(value) else f"{value:.{digits}f}"
