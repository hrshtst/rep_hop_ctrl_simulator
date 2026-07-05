"""Time-series CSV input/output in the simulator's logger schema.

The file format is defined by the C library's default logger
(``simulator_header_default`` plus ``ctrl_dynmorph_header``), i.e. what
``graph/make_time_series.sh`` produces and what interactive sessions
export: a header line starting with ``tag`` followed by one row per
sample. The parser is defined against that schema, not against any
particular generator script, so the app stays self-contained.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

# Columns that the C writer prints as integers (%d).
INT_COLUMNS = frozenset({"n", "phase", "type", "soft_landing"})

# The bare minimum needed to replay anything at all.
REQUIRED_COLUMNS = ("t", "z", "vz")


class CsvFormatError(ValueError):
    """The file does not follow the logger CSV schema."""


def read_timeseries(path: str | Path) -> dict[str, np.ndarray]:
    """Read a logger-schema CSV into a column dictionary.

    Returns a mapping of column name to 1-D array (float, except the
    integer columns). The ``tag`` column is dropped. Raises
    :class:`CsvFormatError` if the required columns are missing.
    """
    with Path(path).open(encoding="utf-8") as fp:
        header = fp.readline().strip()
    if not header:
        msg = f"{path}: empty file"
        raise CsvFormatError(msg)
    names = header.split(",")
    missing = [c for c in REQUIRED_COLUMNS if c not in names]
    if missing:
        msg = f"{path}: not a logger-schema CSV (missing columns: {', '.join(missing)})"
        raise CsvFormatError(msg)

    numeric = [(i, name) for i, name in enumerate(names) if name != "tag"]
    data = np.loadtxt(
        path,
        delimiter=",",
        skiprows=1,
        usecols=[i for i, _ in numeric],
        ndmin=2,
    )
    if data.shape[0] == 0:
        msg = f"{path}: no data rows"
        raise CsvFormatError(msg)

    columns: dict[str, np.ndarray] = {}
    for j, (_, name) in enumerate(numeric):
        col = data[:, j]
        columns[name] = col.astype(int) if name in INT_COLUMNS else col
    return columns


def write_timeseries(path: str | Path, header: str, columns: dict[str, np.ndarray], tag: str) -> int:
    """Write columns to ``path`` in the exact logger schema.

    ``header`` is the header line produced by the bindings'
    ``csv_header()`` (starting with ``tag``); every non-tag name in it
    must be present in ``columns``. Returns the number of rows written.
    """
    names = header.strip().split(",")
    if names[0] != "tag":
        msg = "header must start with 'tag'"
        raise CsvFormatError(msg)
    missing = [n for n in names[1:] if n not in columns]
    if missing:
        msg = f"columns missing for header fields: {', '.join(missing)}"
        raise CsvFormatError(msg)

    arrays = [np.asarray(columns[n]) for n in names[1:]]
    n_rows = len(arrays[0])
    if any(len(a) != n_rows for a in arrays):
        msg = "all columns must have the same length"
        raise CsvFormatError(msg)

    # Match the C writer's formatting: %f for floats, %d for ints.
    fmt = tag + "," + ",".join("%d" if n in INT_COLUMNS else "%f" for n in names[1:])
    np.savetxt(path, np.column_stack(arrays), fmt=fmt, comments="", header=header.strip())
    return n_rows


def concat_records(chunks: list[dict[str, np.ndarray]]) -> dict[str, np.ndarray]:
    """Concatenate a list of record dictionaries column-wise."""
    if not chunks:
        return {}
    return {name: np.concatenate([c[name] for c in chunks]) for name in chunks[0]}
