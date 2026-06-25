# rhc — Python bindings

pybind11 bindings for the `rep_hop_ctrl_simulator` C library. The extension
compiles the library's C sources (`../../src`) directly, so no separate shared
library install is required.

```python
import rhc
print(rhc.__version__)
```

Built and managed as part of the `python/` uv workspace.
