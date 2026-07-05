"""Replay source: drives the widgets from a recorded time-series CSV.

Presents the same ``frame()`` / pause / step / reset interface as the
live engine, so the window code is identical in both modes. The cursor
either follows the wall clock (replay mode) or is positioned explicitly
with :meth:`seek_time` (headless rendering).
"""

from __future__ import annotations

import time
from typing import TYPE_CHECKING

import numpy as np

from rhc_demo.engine import STEP_INCREMENT
from rhc_demo.state import Snapshot

if TYPE_CHECKING:
    from pathlib import Path


def _column(data: dict[str, np.ndarray], name: str, default: float) -> np.ndarray:
    n = len(data["t"])
    col = data.get(name)
    return col if col is not None else np.full(n, default)


class ReplaySource:
    """Steps a cursor through logged samples at wall-clock rate."""

    def __init__(self, data: dict[str, np.ndarray], speed: float = 1.0) -> None:
        self._data = data
        self.speed = speed
        self._t = data["t"]
        self._z = data["z"]
        self._vz = data["vz"]
        self._fz = _column(data, "fz", 0.0)
        self._fe = _column(data, "fe", 0.0)
        self._phase = _column(data, "phase", -1.0)
        self._hops = _column(data, "n", 0.0)
        nan = float("nan")
        self._za = _column(data, "za", nan)
        self._zh = _column(data, "zh", nan)
        self._zm = _column(data, "zm", nan)
        self._zb = _column(data, "zb", nan)
        self._rho = _column(data, "rho", nan)
        self._k = _column(data, "k", nan)
        self._soft_landing = _column(data, "soft_landing", 1.0)
        self._p_za = _column(data, "p_za", nan)
        self._p_zm = _column(data, "p_zm", nan)
        self._p_zb = _column(data, "p_zb", nan)
        self._p_rho = _column(data, "p_rho", nan)

        self._idx = 0
        self._emitted = 0  # samples already handed to the UI
        self._paused = False
        self._clock: float | None = None
        self._time = float(self._t[0])

    @property
    def duration(self) -> float:
        return float(self._t[-1] - self._t[0])

    @property
    def finished(self) -> bool:
        return self._idx >= len(self._t) - 1

    # -- transport controls ---------------------------------------------------
    def start(self) -> None:
        self._clock = time.monotonic()

    def close(self) -> None:
        """Nothing to shut down; provided for interface symmetry."""

    def set_paused(self, paused: bool) -> None:
        if not paused:
            self._clock = time.monotonic()
        self._paused = paused

    def request_step(self, duration: float = STEP_INCREMENT) -> None:
        """While paused, move the cursor forward by ``duration`` seconds."""
        self._time = min(self._time + duration, float(self._t[-1]))

    def reset(self) -> None:
        self._idx = 0
        self._emitted = 0
        self._time = float(self._t[0])
        self._clock = time.monotonic()

    def seek_time(self, t: float) -> None:
        """Position the cursor at absolute sample time ``t`` (headless use)."""
        self._time = min(max(t, float(self._t[0])), float(self._t[-1]))

    # -- frame production -------------------------------------------------------
    def frame(self) -> tuple[Snapshot, list[tuple[np.ndarray, np.ndarray]]]:
        now = time.monotonic()
        if not self._paused and self._clock is not None:
            self._time = min(self._time + (now - self._clock) * self.speed, float(self._t[-1]))
        self._clock = now

        self._idx = int(np.searchsorted(self._t, self._time, side="right") - 1)
        self._idx = max(self._idx, 0)

        chunks: list[tuple[np.ndarray, np.ndarray]] = []
        if self._idx + 1 > self._emitted:
            sl = slice(self._emitted, self._idx + 1)
            chunks.append((self._z[sl], self._vz[sl]))
            self._emitted = self._idx + 1
        return self._snapshot_at(self._idx), chunks

    def _snapshot_at(self, i: int) -> Snapshot:
        return Snapshot(
            t=float(self._t[i]),
            z=float(self._z[i]),
            vz=float(self._vz[i]),
            fz=float(self._fz[i]),
            fe=float(self._fe[i]),
            phase=int(self._phase[i]),
            hops=int(self._hops[i]),
            za=float(self._za[i]),
            zh=float(self._zh[i]),
            zm=float(self._zm[i]),
            zb=float(self._zb[i]),
            rho=float(self._rho[i]),
            k=float(self._k[i]),
            soft_landing=bool(self._soft_landing[i]),
            p_za=float(self._p_za[i]),
            p_zm=float(self._p_zm[i]),
            p_zb=float(self._p_zb[i]),
            p_rho=float(self._p_rho[i]),
            playing=not self._paused and not self.finished,
            finished=self.finished,
        )


def load_replay(path: str | Path, speed: float = 1.0) -> ReplaySource:
    """Read a logger-schema CSV and wrap it in a :class:`ReplaySource`."""
    from rhc_demo.csv_io import read_timeseries

    return ReplaySource(read_timeseries(path), speed=speed)
