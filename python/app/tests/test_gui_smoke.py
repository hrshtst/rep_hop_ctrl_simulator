from __future__ import annotations

import pytest
from PyQt6.QtWidgets import QApplication

from rhc_demo.main_window import MainWindow


@pytest.fixture
def qt_app():
    return QApplication.instance() or QApplication([])


def test_window_constructs_and_ticks(qt_app):
    win = MainWindow()
    for _ in range(5):
        win.tick()
    snap = win.driver.snapshot()
    assert snap.t > 0.0
    assert win.phase is not None
    assert win.biped is not None


def test_reset_returns_to_start(qt_app):
    win = MainWindow()
    for _ in range(5):
        win.tick()
    win._on_reset()
    assert win.driver.snapshot().t == 0.0


def test_rho_slider_updates_driver(qt_app):
    win = MainWindow()
    win._on_rho(0.0)
    assert win.driver.params.rho == 0.0
