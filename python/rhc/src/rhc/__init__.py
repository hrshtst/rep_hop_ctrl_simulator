"""Python bindings for the rep_hop_ctrl_simulator C library.

The compiled extension :mod:`rhc._rhc` exposes a single high-level facade,
:class:`DynmorphSim`, that owns the whole coupled C system (command, model,
dynamics-morphing controller and integrator). Stepping is batched and runs
with the GIL released; per-substep samples are returned as NumPy arrays
that own their buffers without copying.
"""

from __future__ import annotations

from rhc._rhc import (
    DynmorphSim,
    DynmorphType,
    Phase,
    __version__,
)

__all__ = [
    "DynmorphSim",
    "DynmorphType",
    "Phase",
    "__version__",
]
