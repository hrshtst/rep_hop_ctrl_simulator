"""Headless mode: render a replay offscreen into frames, then MP4/GIF.

No window is ever shown: the caller must ensure Qt runs with the
``offscreen`` platform plugin (the entry point sets QT_QPA_PLATFORM
before creating the QApplication — under Wayland capturing live windows
is restricted, so offscreen rendering is mandatory, not an optimization).
Frames are produced deterministically by seeking the replay cursor to
each frame time and grabbing the widget, so output does not depend on
wall-clock scheduling. Videos are encoded with the ffmpeg binary bundled
by :mod:`imageio_ffmpeg`.
"""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Callable

    from rhc_demo.replay import ReplaySource

DEFAULT_FPS = 50
DEFAULT_SIZE = (1280, 720)
GIF_FPS = 25
GIF_WIDTH = 960


def render_frames(
    source: ReplaySource,
    out_dir: str | Path,
    fps: int = DEFAULT_FPS,
    size: tuple[int, int] = DEFAULT_SIZE,
    speed: float = 1.0,
    progress: Callable[[int, int], None] | None = None,
) -> list[Path]:
    """Render the whole replay to PNG frames; returns the frame paths.

    ``speed`` scales playback (2.0 = twice as fast). Imports Qt lazily so
    the module can be imported without a display or QApplication.
    """
    from rhc_demo.main_window import MainWindow

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    window = MainWindow(source)
    window._timer.stop()  # noqa: SLF001 — headless drives frames explicitly
    window.resize(*size)
    source.set_paused(True)

    duration = source.duration / speed
    n_frames = max(int(duration * fps) + 1, 1)
    t0 = 0.0
    paths: list[Path] = []
    for i in range(n_frames):
        source.seek_time(t0 + i / fps * speed)
        window.render_frame()
        # Let the curve worker finish the initial portrait on early frames.
        pixmap = window.grab()
        path = out_dir / f"frame_{i:06d}.png"
        pixmap.save(str(path), "PNG")
        paths.append(path)
        if progress is not None:
            progress(i + 1, n_frames)
    window.close()
    return paths


def _ffmpeg_exe() -> str:
    from imageio_ffmpeg import get_ffmpeg_exe

    return get_ffmpeg_exe()


def encode_mp4(frame_dir: str | Path, out_file: str | Path, fps: int = DEFAULT_FPS) -> Path:
    """Compile the rendered frame sequence into an H.264 MP4."""
    out_file = Path(out_file)
    cmd = [
        _ffmpeg_exe(),
        "-y",
        "-framerate",
        str(fps),
        "-i",
        str(Path(frame_dir) / "frame_%06d.png"),
        "-c:v",
        "libx264",
        "-pix_fmt",
        "yuv420p",
        "-crf",
        "18",
        str(out_file),
    ]
    subprocess.run(cmd, check=True, capture_output=True)  # noqa: S603 — fixed args, bundled ffmpeg
    return out_file


def encode_gif(frame_dir: str | Path, out_file: str | Path, fps: int = DEFAULT_FPS) -> Path:
    """Compile the frames into a palette-optimized GIF."""
    out_file = Path(out_file)
    filters = f"fps={GIF_FPS},scale={GIF_WIDTH}:-1:flags=lanczos"
    cmd = [
        _ffmpeg_exe(),
        "-y",
        "-framerate",
        str(fps),
        "-i",
        str(Path(frame_dir) / "frame_%06d.png"),
        "-vf",
        f"{filters},split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse",
        str(out_file),
    ]
    subprocess.run(cmd, check=True, capture_output=True)  # noqa: S603 — fixed args, bundled ffmpeg
    return out_file
