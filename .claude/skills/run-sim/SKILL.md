---
name: run-sim
description: Build, run, and plot a hopping-control example simulation in this repo. Use when asked to run a simulation, run an example controller (raibert, slip, mtoka, regulator, dynmorph, etc.), or see/plot its output. Wraps example/run_example.sh.
disable-model-invocation: true
---

Run an example program and plot its output, using `example/run_example.sh`.

Usage: `/run-sim <program> [extra args]` where `<program>` is an example name
(without `.c`), e.g. `slip_test`, `raibert_test`, `mtoka_test`, `regulator_test`,
`slip_var_stiff_test`, `dynmorph_test`. Arguments after the name in `$ARGUMENTS`
are passed through to the script / example program.

Steps:
1. From the repo root, run the example via its script:
   `cd example && ./run_example.sh $ARGUMENTS`
   The script builds (`make`), runs `<program>`, writes `<program>.csv`, then plots
   it with the matching `<program>.py` in `example/graph_plot/`.
2. Report what was produced (the CSV path and whether a plot was generated).

Useful flags the script accepts (pass them as part of `$ARGUMENTS`):
- `--no-plot` — run the program only, skip plotting (use in headless sessions).
- `-P` / `--plot-only` — re-plot existing CSV without re-running the program.
- `-D` / `--delete` — delete the intermediate CSV after plotting.
- `-p <script>` / `--plot-script=<script>` — use a specific plotting script.
- `-d <file>` / `--data=<file>` — use a specific intermediate data file.

Notes:
- If no `$ARGUMENTS` are given, ask which example program to run (list the names above).
- Plotting opens a window; in a non-interactive/headless session prefer `--no-plot`
  and inspect the generated CSV directly.
- Python plotting deps are managed with `uv` in `example/graph_plot/`.
