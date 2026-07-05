# rhc — Python bindings

pybind11 bindings for the `rep_hop_ctrl_simulator` C library. The extension
compiles the library's C sources (`../../src`) directly, so no separate shared
library install is required.

The API is one high-level facade, `DynmorphSim`, owning the whole coupled C
system (cmd + model + dynamics-morphing controller + RK4 integrator). Stepping
is batched and runs with the GIL released; per-substep samples come back as
NumPy arrays that own their buffers without copying, covering the exact CSV
schema of the C pipeline's logger.

```python
import rhc

sim = rhc.DynmorphSim()          # paper defaults; stand start (0.2575, 0)
sim.rho = 1.0                    # morph to hopping
out = sim.advance(50_000, 1e-4)  # 5 s; dict of NumPy columns
print(out["ap_z"][-1], sim.hops) # apex -> 0.28, ~24 hops

curves = sim.solution_curves([(0.24, 0.0), (0.30, 0.5)])  # phase portrait
header = sim.csv_header()        # exact logger header, starts with "tag"
```

Built and managed as part of the `python/` uv workspace.
