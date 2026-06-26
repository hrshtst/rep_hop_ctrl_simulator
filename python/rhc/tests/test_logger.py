from __future__ import annotations

import rhc


def test_logger_open_close(tmp_path):
    log = rhc.Logger()
    assert log.is_open is False
    path = tmp_path / "out.csv"
    log.open(str(path))
    assert log.is_open is True
    assert log.filename == str(path)
    log.close()
    assert log.is_open is False


def test_logger_open_failure_raises():
    log = rhc.Logger()
    try:
        log.open("/no/such/directory/out.csv")
    except RuntimeError:
        return
    msg = "expected RuntimeError opening an unwritable path"
    raise AssertionError(msg)
