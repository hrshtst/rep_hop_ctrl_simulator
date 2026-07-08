## Overview of the Dynamics Morphing Controller App

Here is an overview of the Dynamics Morphing Controller application based on the directive. This breakdown is designed to help an onboarding engineer quickly grasp the project's scope, architecture, and expected functionality.

### **Purpose & Demonstration**

The primary goal of this application is to demonstrate a **dynamics morphing controller** that seamlessly transitions a 1-DOF robotic model between two states: **standing** (regulator) and **repetitive hopping** (oscillator).

It is being built to generate interactive demonstrations and video animations (MP4/GIF) to support a paper revision. By allowing real-time, interactive parameter adjustments and external force injections, the app showcases the controller's robustness and the continuous mathematical morphing between the two dynamic states.

### **Implementation Overview**

The application is a ground-up rewrite on a new Git branch, discarding an older, poorly performing prototype (located on `dev/demo-app-recording`).

* **Frontend (GUI):** Built with **PyQt6**. All new frontend code belongs in `@python/app/src/rhc_demo/`.
* **Backend (Physics/Math):** Validated C++ core logic housed in `@src/` and `@include/`.
* **Bindings (C++ to Python):** Custom **Pybind11** wrappers to be written from scratch (reference older bindings in `@python/rhc/src/bindings/` but do not reuse them).
* **Performance Requirements:** * Must achieve ≥ 100× real-time in headless mode (using a 0.1 ms integration step).
* Must sustain 60 FPS in the UI during interactive mode without blocking the physics engine.
* Requires zero-copy data passing (via NumPy views) between C++ and Python for time-series data and state buffers.


* **Target Environments:** Ubuntu 26.04 (Wayland native) and Ubuntu on WSL2 (WSLg).

### **Execution Modes**

The application must support three distinct pipelines:

1. **Interactive Mode:** Spawns a live simulation window. The simulation starts in a "stand" state: $(z, \dot{z}) = ((z_h + z_m)/2, 0)$. Users can drag sliders to adjust the morphing parameter $\tilde{\rho}$ (where $0$ is standing and $1$ is hopping) and target heights. Users can also click and drag the robot's Center of Mass (COM) to apply a vertical external force ($f_e$). Sessions can be exported to a CSV logger format.
2. **Replay Mode:** Reads a generated time-series CSV (matching the internal logger schema) and replays the simulation in the GUI without running the live physics engine.
3. **Headless Mode:** Runs the simulation without a visible window. It forces the Qt `offscreen` platform plugin, captures frames using `QWidget.grab()`, and uses `ffmpeg-python` to compile the image sequence into MP4s or GIFs for publication.

### **GUI Layout & Components**

The UI follows a strict 3-pane horizontal layout:

* **Phase Portrait View (Left):** Plots vertical COM dynamics ($z$ vs $\dot{z}$). Features a white background, solid black axes, gray solution curves, and dotted boundaries for target apex ($\tilde{z}_a$) and lower kinematic limit ($\tilde{z}_b$). The current state is a red circle. Includes a toggle to show COM history as either a solid continuous line or a fading discrete trail of circles.
* **Robot View (Center):** A 2D schematic of the legged robot (knee bending right). The COM is a red Secchi disk. Dynamic arrows visualize ground reaction force (blue) and user-applied external vertical force (purple). Visual ground contact strictly syncs with the physical phase logic.
* **Control Panel (Right):** Contains sliders for the morphing parameter ($\tilde{\rho}$) and target heights ($\tilde{z}_a, \tilde{z}_m, \tilde{z}_b$). The UI logic must actively enforce kinematic safety constraints (e.g., $\tilde{z}_b < \tilde{z}_m < z_h$). Includes a toggle for "soft landing" and buttons to Pause, Reset, and Step the simulation.

### **Expected Launch Commands & Environment Flags**

While the exact Python entry point (e.g., `main.py`) will be defined during your development, the directive dictates specific environment variable usage to handle cross-platform rendering and the offscreen requirement.

**Standard Interactive/Replay Launch (Wayland/X11/WSLg):**
Rely on Qt's automatic platform detection, but support standard overrides.

```bash
# Default interactive mode
python -m rhc_demo.main

# Replay mode (passing a CSV)
python -m rhc_demo.main --mode replay --data graph/data/sample.csv

# Forcing a specific Qt display backend for testing
QT_QPA_PLATFORM=wayland python -m rhc_demo.main

```

**Headless Video Generation Launch:**
Because capturing live windows is restricted under Wayland, headless mode explicitly requires the offscreen platform.

```bash
# Generates MP4/GIF without rendering a window to the screen
QT_QPA_PLATFORM=offscreen python -m rhc_demo.main --mode headless --data graph/data/sample.csv --output demo_video.mp4

```

---

# Design Modification Plan

Proposed GUI design modifications for the Python demo application, Dynamics Morphing Controller.

## Frontend Design Modifications

### Phase Portrait View

* Increase the width of this view's pane to make it closer to a square.
* Increase the upper limit of the horizontal axis to account for the widened pane. The lower limit does not need to be changed.
* Slightly expand the upper and lower limits of the vertical axis.
* When $\rho > \exp\left(-\frac{1}{(q+1)^2}\right)$, a non-equilibrium stable limit cycle appears in the stance phase. Plot this limit cycle as a solid black line on the Phase Portrait.
  * Check if the functionality to calculate this limit cycle is already implemented in the backend (`@simulator/src/` and `@simulator/include/`).
  * If it is not implemented, add the computation functionality to the backend and its Python binding.
  * Render the limit cycle plot *after* the solution curves to ensure the limit cycle is not obscured.

### Robot View

* Expand the rendering area above the robot.
* Enlarge the Center of Mass (COM) mark rendered with the Secchi disk pattern.
  * Currently, COM mark is rendered exactly on a joint between the body and thigh. Slightly render it upward to make this joint visible.
* Make the Body link longer and thinner.
* Draw the thigh, shank, and foot links as rounded rectangles, similar to the body link.
* Enlarge the rendered joints.
* Plot a dotted line representing $\tilde{z}_b$, similar to the existing line for $\tilde{z}_a$.
* **Regarding the arrows representing $f_z$ and $f_e$:**
  * Lengthen the arrows and enlarge their heads.
  * Keep the magnitude (representing N) labels as fixed in place as possible.
    * Fix the $f_z$ label position near the ground.
    * Draw the $f_e$ label near the COM for better visibility.
  * Decrease the mouse sensitivity when applying $f_e$ via a mouse drag in Interactive Mode. This means users will need to make larger mouse movements to generate forces.

### Control Panel

* Add a slider for parameter $q$. Placing it below parameter $k$ is preferred.
* Add a grouped set of radio buttons to enable slow-motion playback.
  * Allow users to select between **1.0x**, **0.75x**, **0.5x**, and **0.25x** playback speeds.
* In Interactive Mode, data intended for export is held in memory. Ensure that this memory is freed when the **Reset** button is pressed. In other words, the exported data should only contain the session history recorded *after* the most recent reset.
