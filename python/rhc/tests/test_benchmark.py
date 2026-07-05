"""Performance benchmarks from the implementation directive.

Targets: headless stepping through the bindings at >= 100x real time at
the app's 1 kHz recording rate (and comfortably above real time even
with full 10 kHz recording). Thresholds leave headroom for slow CI
machines; run with ``-s`` to see the measured figures.
"""

from __future__ import annotations

import time

import rhc

DT = 1e-4


def _steps_per_second(record_every: int, n: int = 500_000) -> float:
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    sim.advance(10_000, DT)  # warm up into the limit cycle
    t0 = time.perf_counter()
    sim.advance(n, DT, record_every)
    return n / (time.perf_counter() - t0)


def test_stepping_meets_realtime_target():
    sps = _steps_per_second(record_every=10)
    ratio = sps * DT
    print(f"\n1 kHz recording: {sps / 1e6:.2f} Msteps/s = {ratio:.0f}x real time")
    assert ratio >= 50, f"stepping too slow: {ratio:.0f}x real time (directive target: 100x)"


def test_full_recording_still_far_beyond_realtime():
    sps = _steps_per_second(record_every=1)
    ratio = sps * DT
    print(f"\nfull recording: {sps / 1e6:.2f} Msteps/s = {ratio:.0f}x real time")
    assert ratio >= 20
