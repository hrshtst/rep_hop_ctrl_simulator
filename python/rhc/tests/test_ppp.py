from __future__ import annotations

import rhc


def test_ppp_runs_and_writes(tmp_path):
    cmd = rhc.Cmd()
    model = rhc.Model(10.0)
    ctrl = rhc.DynmorphCtrl(cmd, model, rhc.DynmorphType.SOFT_LANDING_PIECEWISE)
    cmd.set(0.28, 0.26, 0.255, 0.24)
    ctrl.set_rho(1.0)

    log = rhc.Logger()
    path = tmp_path / "ppp.csv"
    log.open(str(path))

    ppp = rhc.PhasePortraitPlotter(cmd, ctrl, model, log)
    ppp.set_lim_xy(0.16, 0.36, -1.5, 1.5)
    ppp.set_n_sc_xy(3, 3)
    ppp.push_p0(rhc.Vec([0.255 - 1e-6, 0.0]))
    ppp.push_p0(rhc.Vec([0.24, 0.0]))
    ppp.run(0.3, 1e-3)
    log.close()

    assert path.exists()
    assert path.stat().st_size > 0
