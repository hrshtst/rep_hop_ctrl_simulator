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

## Setup

```sh
cd python
uv sync          # builds the rhc extension and installs both packages
```

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
