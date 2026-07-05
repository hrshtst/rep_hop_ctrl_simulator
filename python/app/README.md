# rhc-demo

PyQt6 application demonstrating the dynamics-morphing hopping controller from the
paper *"Seamless Control Between Standing and Repetitive Hopping for Legged Robots
Based on Dynamics Morphing"*, built on the `rhc` Python bindings.

Layout: **Phase Portrait | Robot View | Control Panel**. Physics runs on its own
thread through the batched bindings; the Qt thread never blocks the simulation.

```sh
uv run rhc-demo                                # interactive
uv run rhc-demo replay data.csv                # replay a logger-schema CSV
uv run rhc-demo headless data.csv -o out --mp4 --gif   # offscreen frames + video
```

See `python/README.md` for the full mode reference.
