# python — bindings and demo for the dynamics-morphing hopping controller

A `uv` workspace with two packages:

- **`rhc/`** — pybind11 bindings of the `rep_hop_ctrl_simulator` C library. The
  extension compiles the library's C sources (`../src/*.c`) directly, so no
  separate shared-library install is needed.
- **`app/`** (`rhc-demo`) — a PyQt6 application that demonstrates the controller
  from the paper *"Seamless Control Between Standing and Repetitive Hopping for
  Legged Robots Based on Dynamics Morphing"*: a 2D biped animation of COM height
  and contact state, a live `(z, ż)` phase portrait, and sliders that morph **ρ**
  between standing (ρ=0, an equilibrium point) and hopping (ρ=1, a limit cycle)
  and retarget the apex/standing/crouching heights on the fly.

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

```sh
uv run rhc-demo
```

Drag **ρ** from 1 → 0 to morph from hopping to standing; change **z̃a** to retarget
the apex live; press **Disturb ↑** to kick the COM during flight and watch the
soft landing; **Reset** restarts.

## Test and lint

```sh
uv run pytest          # binding + app tests
uv run ruff check .
```

The phase portrait and rollouts are computed entirely in memory through the
bindings (no CSV); the C library is built and tested separately with `make`
from the repository root.
