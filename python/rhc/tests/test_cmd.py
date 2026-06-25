from __future__ import annotations

import rhc


def test_default_init():
    cmd = rhc.Cmd()
    assert cmd.za == 0.28
    assert cmd.zh == 0.26
    assert cmd.zm == 0.255
    assert cmd.zb == 0.23


def test_set():
    cmd = rhc.Cmd()
    cmd.set(0.30, 0.26, 0.255, 0.24)
    assert cmd.za == 0.30
    assert cmd.zh == 0.26
    assert cmd.zm == 0.255
    assert cmd.zb == 0.24


def test_writable_properties():
    cmd = rhc.Cmd()
    cmd.za = 0.32
    cmd.zb = 0.22
    assert cmd.za == 0.32
    assert cmd.zb == 0.22


def test_copy_is_independent():
    cmd = rhc.Cmd()
    other = cmd.copy()
    other.za = 0.5
    assert cmd.za == 0.28
    assert other.za == 0.5
