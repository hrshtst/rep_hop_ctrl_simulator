"""CSV schema round-trip tests against the C logger format."""

from __future__ import annotations

import numpy as np
import pytest

import rhc
from rhc_demo.csv_io import CsvFormatError, concat_records, read_timeseries, write_timeseries


@pytest.fixture
def session(tmp_path):
    """A short simulated session exported to CSV."""
    sim = rhc.DynmorphSim()
    sim.rho = 1.0
    chunks = [sim.advance(500, 1e-4, 10) for _ in range(4)]
    columns = concat_records(chunks)
    path = tmp_path / "session.csv"
    rows = write_timeseries(path, sim.csv_header(), columns, tag="test")
    return path, columns, rows


def test_round_trip(session):
    path, columns, rows = session
    assert rows == 200
    data = read_timeseries(path)
    assert len(data["t"]) == rows
    np.testing.assert_allclose(data["t"], columns["t"], atol=1e-6)
    np.testing.assert_allclose(data["z"], columns["z"], atol=1e-6)
    np.testing.assert_allclose(data["fz"], columns["fz"], atol=1e-5)
    assert data["phase"].dtype.kind == "i"
    np.testing.assert_array_equal(data["soft_landing"], columns["soft_landing"])


def test_header_first_field_is_tag(session):
    path, _, _ = session
    header = path.read_text().splitlines()[0]
    assert header.startswith("tag,t,z,vz,m,az,fe,fz,za,zh,zm,zb,vh,n,phi,phase,")
    assert header.endswith(",type,rho,k,q_scale,soft_landing,q1,q2,vm,p_za,p_zh,p_zm,p_zb,p_rho")


def test_rows_carry_the_tag(session):
    path, _, _ = session
    first_row = path.read_text().splitlines()[1]
    assert first_row.startswith("test,")


def test_missing_required_columns_rejected(tmp_path):
    bad = tmp_path / "bad.csv"
    bad.write_text("a,b,c\n1,2,3\n")
    with pytest.raises(CsvFormatError, match="missing columns"):
        read_timeseries(bad)


def test_empty_file_rejected(tmp_path):
    empty = tmp_path / "empty.csv"
    empty.write_text("")
    with pytest.raises(CsvFormatError, match="empty"):
        read_timeseries(empty)


def test_write_rejects_missing_columns(tmp_path):
    with pytest.raises(CsvFormatError, match="columns missing"):
        write_timeseries(tmp_path / "x.csv", "tag,t,z", {"t": np.zeros(3)}, tag="x")


def test_pipeline_csv_compatible(tmp_path):
    """A file in the exact C-pipeline format parses correctly."""
    header = "tag,t,z,vz,m,az,fe,fz,za,zh,zm,zb,vh,n,phi,phase"
    rows = ["sim0,%f,0.2575,0.0,10,0,0,98.0,0.28,0.26,0.255,0.23,0.626,0,1.57,1" % (i * 1e-3) for i in range(5)]
    path = tmp_path / "pipeline.csv"
    path.write_text(header + "\n" + "\n".join(rows) + "\n")
    data = read_timeseries(path)
    assert len(data["t"]) == 5
    assert data["phase"][0] == 1
    assert "tag" not in data
