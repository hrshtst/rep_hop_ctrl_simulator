# rep\_hop\_ctrl\_simulator

A simulator for **repetitive hopping control**. It provides a small C library
that models hopping dynamics (e.g. the Spring-Loaded Inverted Pendulum, SLIP)
together with a collection of controllers, plus example programs that run a
simulation and Python scripts that plot the results.

> This project is still a work in progress.

## Requirements

- A C toolchain: `gcc` and GNU `make`
- Linux (the code links `-lrt` and the test timer uses `CLOCK_MONOTONIC_RAW`)
- For plotting: Python 3.10+ and [`uv`](https://docs.astral.sh/uv/)
- Optional: `valgrind` for the memory-check test mode

## Building

```shell
make          # build the unit tests and all example programs
make clean    # remove build artifacts
```

Example binaries are produced in the `example/` directory.

## Running the tests

```shell
make test               # build and run the full unit-test suite
make test MEMCHECK=y    # same, under valgrind (--leak-check=full)
```

To run a single test, build and execute it directly:

```shell
cd test
make rhc_ctrl_raibert_test
./rhc_ctrl_raibert_test
```

The suite uses the custom, dependency-free harness in
`include/rhc_test.h` (MinUnit-style `TEST` / `ASSERT_*` macros).

## Running an example

Each program in `example/` runs a simulation and writes a CSV file. The
`run_example.sh` helper builds the program, runs it, and plots its output with
the matching script in `example/graph_plot/`:

```shell
cd example
./run_example.sh slip_test          # build, run, and plot
./run_example.sh slip_test --no-plot  # run only (e.g. headless)
./run_example.sh -P slip_test         # re-plot existing CSV without re-running
```

Run `./run_example.sh --help` for all options.

Available example programs:

| Program | Description |
|---|---|
| `slip_test` | SLIP hopping model |
| `slip_var_stiff_test` | SLIP with variable leg stiffness |
| `raibert_test` | Raibert-style hopping controller |
| `regulator_test` | Regulator / stabilizing controller |
| `mtoka_test`, `mtoka_osci_test` | Matsuoka-oscillator-based control |
| `arl_test` | ARL controller |
| `dynmorph_test` | Dynamic morphing with soft-landing strategies |
| `euler_test`, `rk4_test` | ODE integrator demos (Euler / Runge-Kutta 4) |
| `ppp_test` | Phase-portrait plotting demo |

The controllers live in `include/rhc_ctrl_*.h` and `src/rhc_ctrl_*.c`.

## Python bindings & demo app

The `python/` directory is a [`uv`](https://docs.astral.sh/uv/) workspace with
pybind11 bindings of the C library (`rhc`) and a PyQt6 application (`rhc-demo`)
that demonstrates the dynamics-morphing standing/hopping controller from the
paper in `resources/`: a 2D biped animation of COM height and contact state, a
live `(z, ż)` phase portrait, and sliders that morph **ρ** between standing
(ρ=0, an equilibrium point) and hopping (ρ=1, a limit cycle).

```shell
cd python
uv sync          # builds the rhc extension and installs both packages
uv run rhc-demo  # launch the demo
```

Drag **ρ** from 1 → 0 to morph from hopping to standing, change **z̃a** to
retarget the apex live, press **Disturb ↑** to perturb the COM in flight, and
**Reset** to restart. Run the Python tests with `uv run pytest`. See
[`python/README.md`](python/README.md) for details.

## Project layout

```
include/      public API headers (rhc_*.h)
src/          library implementation (rhc_*.c)
test/         unit tests and the run_test.sh runner
example/      example programs and run_example.sh
  graph_plot/ Python plotting scripts (managed with uv)
python/       uv workspace: pybind11 bindings (rhc) and the PyQt6 demo (rhc-demo)
```

## License

Distributed under the terms of the GNU General Public License v3.0. See
[LICENSE](LICENSE).
