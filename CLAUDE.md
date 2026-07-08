# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A simulator for **repetitive hopping control**: a small C library (`include/rhc_*.h`, `src/rhc_*.c`) implementing hopping dynamics (e.g. SLIP) and a set of controllers (`rhc_ctrl_*` — Raibert, regulator, MTOKA oscillator, SLIP variable-stiffness, dynmorph, etc.). `example/` holds demo programs that run a controller and write CSV; `example/graph_plot/` holds Python scripts that plot the CSV. `python/` holds Python bindings of the C library plus a PyQt6 demo app.

## Build & test

- `make` — build everything (tests + examples). Example binaries land in `example/`.
- `make test` — build and run the full unit-test suite. `make test MEMCHECK=y` runs it under Valgrind (`--leak-check=full`).
- `make clean` — remove all build artifacts.
- Run one test: `cd test && make <name>_test && ./<name>_test` (e.g. `rhc_ctrl_dynmorph_test`). Or via the runner: `cd test && ./run_test.sh <name>_test`. Skip one in a full run: `./run_test.sh -s <name>_test`.

## Gotchas

- **`rhc_test_test` always fails by design** and is auto-skipped by `run_test.sh` in a full run. Don't try to "fix" it; if you run it standalone, a failure is expected.
- **Do not reformat C code.** `.clang-format` sets `DisableFormat: true` on purpose. Match the surrounding style by hand.
- **Linux only.** Code links `-lm -lrt` and the test timer uses `CLOCK_MONOTONIC_RAW`.
- Each `test/*_test.c` and `example/*.c` compiles into its own executable, linking every object in `src/`. Editing a `src/` file affects all tests/examples.

## Conventions

- Public API is prefixed `rhc_`; header include guards are `__RHC_<NAME>_H__`.
- Tests use the custom harness in `include/rhc_test.h` (MinUnit-style): `TEST(...)`, `RUN_TEST`, `ASSERT_*`, `TEST_REPORT()`, `TEST_EXIT()`. No external test framework.

## Python plotting (`example/graph_plot/`)

- Managed with `uv`. Lint/format with `ruff` (config in `pyproject.toml`: line length 120, double quotes, `select = ["ALL"]`, numpy-style docstrings).
- Convenience workflow: `example/run_example.sh <program>` builds, runs the example, and plots its CSV using the matching `<program>.py`. See `/run-sim`.

## Python bindings & demo app (`python/`)

- A `uv` workspace with two packages: `rhc/` (pybind11 bindings that compile `src/*.c` directly into the extension; single facade `rhc.DynmorphSim`) and `app/` (`rhc-demo`, a PyQt6 demo of the paper's controller). See `python/README.md`.
- Setup/build: `cd python && uv sync`. Run: `uv run rhc-demo` (also `replay`/`headless` modes). Test: `uv run pytest`. Lint: `uv run ruff check .` (own config in `python/pyproject.toml`; distinct from `example/graph_plot`'s).
- **Gotcha: editing the C library (`src/`, `include/`) or `rhc/src/bindings/` does NOT rebuild the cached extension** — force it with `uv sync --reinstall-package rhc`. Pure-Python edits are editable installs and need no rebuild.
