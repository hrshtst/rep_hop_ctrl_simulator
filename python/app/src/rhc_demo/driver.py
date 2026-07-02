"""Headless simulation driver for the demo.

All physics lives here so it can be unit-tested without a GUI. The driver owns
a :class:`rhc.Simulator` plus its controller/command/model and advances them
step by step, letting control parameters be changed live (matching the paper's
on-the-fly modification and disturbance scenarios).
"""

from __future__ import annotations

from dataclasses import dataclass, field, replace

import rhc

DEFAULT_TYPE = rhc.DynmorphType.SOFT_LANDING_PIECEWISE

# Maximum |drho/dt| applied to the controller. Switching the oscillator on
# instantaneously from a settled stand can demand a peak thrust an order of
# magnitude above the steady hopping force; slewing rho keeps the morph
# continuous no matter how the slider is moved.
RHO_SLEW_RATE = 2.0  # 1/s


@dataclass(frozen=True)
class Params:
    """Controller/model parameters. Frozen so updates produce a new value."""

    za: float = 0.28
    zh: float = 0.26
    zm: float = 0.255
    zb: float = 0.24
    rho: float = 1.0
    k: float = 4.0
    mass: float = 10.0
    soft_landing: bool = True
    ctrl_type: rhc.DynmorphType = field(default=DEFAULT_TYPE)


@dataclass
class Snapshot:
    """A read-only view of the current state for the widgets."""

    t: float
    z: float
    vz: float
    phase: rhc.Phase
    contact: bool
    fz: float
    fz_peak: float
    n: int
    param_za: float
    param_zm: float
    param_zb: float
    param_rho: float


@dataclass
class System:
    """The live C objects, kept together so their references stay alive."""

    cmd: rhc.Cmd
    model: rhc.Model
    ctrl: rhc.DynmorphCtrl
    sim: rhc.Simulator


def make_system(params: Params) -> System:
    """Build a fresh cmd/model/controller/simulator from ``params``."""
    cmd = rhc.Cmd()
    model = rhc.Model(params.mass)
    ctrl = rhc.DynmorphCtrl(cmd, model, params.ctrl_type)
    cmd.set(params.za, params.zh, params.zm, params.zb)
    ctrl.set_rho(params.rho)
    ctrl.set_k(params.k)
    if params.soft_landing:
        ctrl.enable_soft_landing()
    else:
        ctrl.disable_soft_landing()
    sim = rhc.Simulator(cmd, ctrl, model)
    return System(cmd, model, ctrl, sim)


class Driver:
    """Advances a hopping simulation in real time with live parameters."""

    def __init__(
        self,
        params: Params | None = None,
        dt: float = 0.0001,
        z0: float = 0.255,
        vz0: float = 0.0,
    ) -> None:
        self.dt = dt
        self.params = params if params is not None else Params()
        self._z0 = z0
        self._vz0 = vz0
        self._disturb_remaining = 0.0
        self._rebuild()

    def _rebuild(self) -> None:
        self.sys = make_system(self.params)
        self.sys.sim.reset()
        self.sys.sim.set_state(rhc.Vec([self._z0, self._vz0]))
        self._disturb_remaining = 0.0
        self._rho_applied = self.params.rho
        self._fz_peak = 0.0

    def reset(self) -> None:
        """Rebuild the system and return to the initial state."""
        self._rebuild()

    def step(self, n_substeps: int = 1) -> None:
        """Advance the simulation by ``n_substeps`` integration steps."""
        sim = self.sys.sim
        ctrl = self.sys.ctrl
        peak = 0.0
        for _ in range(n_substeps):
            self._slew_rho()
            if self._disturb_remaining > 0.0:
                self._disturb_remaining -= self.dt
                if self._disturb_remaining <= 0.0:
                    sim.fe = 0.0
            sim.step(self.dt)
            peak = max(peak, ctrl.fz)
        self._fz_peak = peak

    def _slew_rho(self) -> None:
        """Move the applied rho toward the commanded one at a bounded rate."""
        target = self.params.rho
        if self._rho_applied == target:
            return
        limit = RHO_SLEW_RATE * self.dt
        delta = min(max(target - self._rho_applied, -limit), limit)
        self._rho_applied += delta
        self.sys.ctrl.set_rho(self._rho_applied)

    # -- live parameter setters (mutate the running system in place) --
    def set_rho(self, value: float) -> None:
        """Command a new rho; it is applied gradually by the slew limiter."""
        self.params = replace(self.params, rho=value)

    def set_k(self, value: float) -> None:
        self.params = replace(self.params, k=value)
        self.sys.ctrl.set_k(value)

    def set_za(self, value: float) -> None:
        self.params = replace(self.params, za=value)
        self.sys.cmd.za = value

    def set_zh(self, value: float) -> None:
        self.params = replace(self.params, zh=value)
        self.sys.cmd.zh = value

    def set_zm(self, value: float) -> None:
        self.params = replace(self.params, zm=value)
        self.sys.cmd.zm = value

    def set_zb(self, value: float) -> None:
        self.params = replace(self.params, zb=value)
        self.sys.cmd.zb = value

    def set_mass(self, value: float) -> None:
        self.params = replace(self.params, mass=value)
        self.sys.model.mass = value

    def set_soft_landing(self, *, enabled: bool) -> None:
        self.params = replace(self.params, soft_landing=enabled)
        if enabled:
            self.sys.ctrl.enable_soft_landing()
        else:
            self.sys.ctrl.disable_soft_landing()

    def apply_disturbance(self, force: float, duration: float = 0.05) -> None:
        """Apply an external vertical force for a short window (e.g. in flight)."""
        self.sys.sim.fe = force
        self._disturb_remaining = duration

    # -- read access --
    def is_in_flight(self) -> bool:
        return self.sys.ctrl.is_in_flight()

    def sim_fe(self) -> float:
        return self.sys.sim.fe

    def snapshot(self) -> Snapshot:
        sim = self.sys.sim
        ctrl = self.sys.ctrl
        return Snapshot(
            t=sim.time,
            z=sim.z,
            vz=sim.vz,
            phase=ctrl.phase,
            contact=ctrl.is_in_contact(),
            fz=ctrl.fz,
            fz_peak=self._fz_peak,
            n=ctrl.n,
            param_za=ctrl.param_za,
            param_zm=ctrl.param_zm,
            param_zb=ctrl.param_zb,
            param_rho=ctrl.param_rho,
        )
