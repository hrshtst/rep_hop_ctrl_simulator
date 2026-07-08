"""Live simulation engine: physics on its own thread, never blocked by the UI.

The engine owns the :class:`rhc.DynmorphSim` facade and advances it on a
dedicated thread, paced against the wall clock. Each pass advances the
simulation in small fixed chunks (1 ms of simulated time) so commanded
parameter changes and the rho slew limiter act with millisecond latency,
while the heavy stepping runs inside the bindings with the GIL released —
the Qt thread only ever exchanges small snapshots and sample buffers
under a lock.
"""

from __future__ import annotations

import queue
import threading
import time
from dataclasses import replace
from typing import TYPE_CHECKING

import rhc
from rhc_demo.csv_io import concat_records, write_timeseries
from rhc_demo.params import RECORD_EVERY, SIM_DT, Params
from rhc_demo.state import Snapshot

if TYPE_CHECKING:
    from pathlib import Path

    import numpy as np

# Integration substeps per chunk; 10 steps at dt=1e-4 is 1 ms of sim time.
CHUNK_STEPS = 10

# Maximum |d rho / dt|. Switching the oscillator on instantaneously from a
# settled stand can demand a peak thrust an order of magnitude above the
# steady hopping force; slewing rho keeps the morph continuous no matter
# how fast the slider is moved. (Lesson carried over from the prototype.)
RHO_SLEW_RATE = 2.0  # 1/s

# Never try to catch up more than this much simulated time in one pass;
# beyond it, wall time is dropped instead (avoids a death spiral after a
# UI stall or suspend).
MAX_CATCHUP = 0.05  # s

# Default sim-time increment of the step-wise advancement button.
STEP_INCREMENT = 0.01  # s

IDLE_SLEEP = 0.002  # s


class LiveEngine:
    """Drives an interactive dynamics-morphing simulation in real time."""

    def __init__(
        self,
        params: Params | None = None,
        dt: float = SIM_DT,
        record_every: int = RECORD_EVERY,
        speed: float = 1.0,
    ) -> None:
        self.dt = dt
        self.record_every = record_every
        self.speed = speed
        self.params = params or Params()
        self._sim = rhc.DynmorphSim(mass=self.params.mass)
        self._apply_params(self.params)
        self._rho_applied = self.params.rho
        self._sim.reset()
        self.csv_header = self._sim.csv_header()

        self._commands: queue.SimpleQueue[tuple[str, object]] = queue.SimpleQueue()
        self._lock = threading.Lock()
        self._records: list[dict[str, np.ndarray]] = []
        self._ui_chunks: list[tuple[np.ndarray, np.ndarray]] = []
        self._paused = False
        self._step_budget = 0.0
        self._running = False
        self._thread: threading.Thread | None = None
        self._snapshot = self._make_snapshot()

    # -- lifecycle ----------------------------------------------------------
    def start(self) -> None:
        if self._thread is not None:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, name="physics", daemon=True)
        self._thread.start()

    def close(self) -> None:
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=2.0)
            self._thread = None

    # -- UI-side API (thread-safe) ------------------------------------------
    def frame(self) -> tuple[Snapshot, list[tuple[np.ndarray, np.ndarray]]]:
        """Return the latest snapshot and the (z, vz) samples since last call."""
        with self._lock:
            chunks = self._ui_chunks
            self._ui_chunks = []
            return self._snapshot, chunks

    def set_param(self, name: str, value: float | bool) -> None:
        self._commands.put(("param", (name, value)))

    def set_fe(self, force: float) -> None:
        """Set the external vertical force (from the robot-view drag)."""
        self._commands.put(("fe", force))

    def set_speed(self, speed: float) -> None:
        """Set the wall-clock playback speed factor (1.0 = real time)."""
        self._commands.put(("speed", speed))

    def pause(self) -> None:
        self._commands.put(("pause", True))

    def resume(self) -> None:
        self._commands.put(("pause", False))

    def set_paused(self, paused: bool) -> None:
        self._commands.put(("pause", paused))

    def request_step(self, duration: float = STEP_INCREMENT) -> None:
        """While paused, advance the simulation by ``duration`` seconds."""
        self._commands.put(("step", duration))

    def reset(self) -> None:
        self._commands.put(("reset", None))

    def export_csv(self, path: str | Path, tag: str = "interactive") -> int:
        """Write the recorded session history to ``path``; returns row count."""
        with self._lock:
            chunks = list(self._records)
        columns = concat_records(chunks)
        if not columns:
            return 0
        return write_timeseries(path, self.csv_header, columns, tag)

    # -- physics thread -------------------------------------------------------
    def _run(self) -> None:
        chunk_time = CHUNK_STEPS * self.dt
        last = time.monotonic()
        debt = 0.0
        while self._running:
            self._drain_commands()
            now = time.monotonic()
            if self._paused:
                last = now
                debt = 0.0
                budget = self._step_budget
                self._step_budget = 0.0
                if budget <= 0.0:
                    time.sleep(IDLE_SLEEP)
                    continue
            else:
                debt = min(debt + (now - last) * self.speed, MAX_CATCHUP)
                last = now
                budget = debt
            n_chunks = int(budget / chunk_time)
            if n_chunks == 0:
                time.sleep(IDLE_SLEEP)
                continue
            if not self._paused:
                debt -= n_chunks * chunk_time
            for _ in range(n_chunks):
                self._advance_chunk()
            self._publish()

    def _advance_chunk(self) -> None:
        self._slew_rho(CHUNK_STEPS * self.dt)
        rec = self._sim.advance(CHUNK_STEPS, self.dt, self.record_every)
        with self._lock:
            self._records.append(rec)
            self._ui_chunks.append((rec["z"], rec["vz"]))

    def _slew_rho(self, dt_chunk: float) -> None:
        target = self.params.rho
        if self._rho_applied == target:
            return
        limit = RHO_SLEW_RATE * dt_chunk
        delta = min(max(target - self._rho_applied, -limit), limit)
        self._rho_applied += delta
        self._sim.rho = self._rho_applied

    def _drain_commands(self) -> None:
        while True:
            try:
                kind, payload = self._commands.get_nowait()
            except queue.Empty:
                break
            if kind == "param":
                name, value = payload
                self._set_param_now(name, value)
            elif kind == "fe":
                self._sim.fe = float(payload)
            elif kind == "speed":
                self.speed = float(payload)
            elif kind == "pause":
                self._paused = bool(payload)
            elif kind == "step":
                self._step_budget += float(payload)
            elif kind == "reset":
                self._reset_now()
        self._publish()

    def _set_param_now(self, name: str, value: float | bool) -> None:
        self.params = replace(self.params, **{name: value})
        if name == "rho":
            return  # applied gradually by the slew limiter
        if name == "mass":
            self._sim.mass = float(value)
        else:
            setattr(self._sim, name, value)

    def _reset_now(self) -> None:
        self._rho_applied = self.params.rho
        self._sim.rho = self._rho_applied
        self._sim.reset()
        self._step_budget = 0.0
        with self._lock:
            self._records = []
            self._ui_chunks = []

    def _publish(self) -> None:
        snap = self._make_snapshot()
        with self._lock:
            self._snapshot = snap

    def _make_snapshot(self) -> Snapshot:
        sim = self._sim
        return Snapshot(
            t=sim.t,
            z=sim.z,
            vz=sim.vz,
            fz=sim.fz,
            fe=sim.fe,
            phase=int(sim.phase.value),
            hops=sim.hops,
            za=sim.za,
            zh=sim.zh,
            zm=sim.zm,
            zb=sim.zb,
            rho=self.params.rho,
            k=sim.k,
            q_scale=sim.q_scale,
            soft_landing=sim.soft_landing,
            p_za=sim.p_za,
            p_zm=sim.p_zm,
            p_zb=sim.p_zb,
            p_rho=sim.p_rho,
            playing=not self._paused,
        )

    def _apply_params(self, p: Params) -> None:
        self._sim.za = p.za
        self._sim.zh = p.zh
        self._sim.zm = p.zm
        self._sim.zb = p.zb
        self._sim.rho = p.rho
        self._sim.k = p.k
        self._sim.q_scale = p.q_scale
        self._sim.soft_landing = p.soft_landing
