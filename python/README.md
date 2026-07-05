# python — bindings and demo for the dynamics-morphing hopping controller

A `uv` workspace with two packages:

- **`rhc/`** — pybind11 bindings of the `rep_hop_ctrl_simulator` C library. The
  extension compiles the library's C sources (`../src/*.c`) directly, so no
  separate shared-library install is needed. The API is a single high-level
  facade, `rhc.DynmorphSim`, that owns the whole coupled C system and steps it
  in batches with the GIL released.
- **`app/`** (`rhc-demo`) — a PyQt6 application that demonstrates the controller
  from the paper *"Seamless Control Between Standing and Repetitive Hopping for
  Legged Robots Based on Dynamics Morphing"*: a live `(z, ż)` phase portrait, a
  2D schematic robot view (knee-right, per the paper's Fig. 2), and a control
  panel whose sliders morph **ρ̃** between standing (ρ=0, an equilibrium point)
  and hopping (ρ=1, a limit cycle) and retarget the apex/standing/crouching
  heights on the fly under automatic kinematic-constraint clamping.

## Building from a clean checkout

Requires [`uv`](https://docs.astral.sh/uv/) and a C/C++ compiler (`gcc`/`g++`);
CMake/Ninja are fetched automatically by the build backend.

```sh
cd python
uv sync
```

`uv sync` creates the virtual environment, compiles the C library sources
(`../src/*.c`) together with the pybind11 bindings into the `rhc._rhc` extension
(via scikit-build-core/CMake), and installs both `rhc` and `rhc-demo` as
editable packages.

## Rebuilding after a C change

The compiled extension is built once and cached. Editing the C library
(`../src/*.c`, `../include/*.h`) or the binding sources (`rhc/src/bindings/*`)
does **not** trigger an automatic rebuild — force one with:

```sh
cd python
uv sync --reinstall-package rhc
```

CMake recompiles only the changed files and the extension is reinstalled.
(Pure-Python edits to the `rhc`/`rhc_demo` packages are editable and need no
rebuild.)

## Run the demo

The application has three execution modes:

```sh
uv run rhc-demo                                  # interactive live simulation
uv run rhc-demo replay data.csv [--speed 1.5]    # replay a recorded time series
uv run rhc-demo headless data.csv -o out/ --mp4 --gif   # offscreen frames + video
```

**Interactive** starts from the paper's stand start `(z, ż) = ((z_h+z_m)/2, 0)`
with the paper's default parameters. Drag **ρ̃** from 0 → 1 to morph from
standing to hopping (the engine slews ρ at a bounded rate, so even a slider
jump morphs continuously); retarget **z̃_a**, **z̃_m**, **z̃_b** live — the
sliders enforce `z̃_b < z̃_m < z_h` and `z̃_b < z̃_a` automatically; click and
drag vertically inside the robot view to apply an external vertical force f_e
(the plant is 1-DOF, so the horizontal drag component is discarded); toggle the
soft-landing strategy; pause/step/reset; and **Export CSV** to save the session
in the simulator's logger schema for later replay or headless rendering.

**Replay** accepts any logger-schema CSV — the files produced by the paper
repository's `graph/make_time_series.sh`, or an exported interactive session.
The parameter widgets are disabled and mirror the logged values.

**Headless** renders the replay offscreen (`QT_QPA_PLATFORM=offscreen`; no
window is shown — under Wayland capturing live windows is restricted, so
offscreen rendering is the supported path) into a deterministic PNG frame
sequence, then optionally encodes MP4/GIF with the ffmpeg binary bundled by
`imageio-ffmpeg`.

Qt platform is auto-detected (native Wayland on GNOME, WSLg, or X11); set
`QT_QPA_PLATFORM` explicitly only to override.

## Test, lint, benchmark

```sh
uv run pytest          # binding + app tests (includes performance benchmarks)
uv run pytest rhc/tests/test_benchmark.py -s   # print measured stepping speed
uv run ruff check .
```

The phase portrait and rollouts are computed entirely in memory through the
bindings (no CSV); the C library is built and tested separately with `make`
from the repository root.
