"""Entry point: interactive, replay, and headless execution modes.

Examples
--------
Interactive live simulation (default)::

    rhc-demo
    rhc-demo interactive

Replay a time-series CSV (e.g. from graph/make_time_series.sh)::

    rhc-demo replay data.csv --speed 1.5

Render a CSV offscreen to frames plus MP4/GIF::

    rhc-demo headless data.csv -o out/ --fps 50 --mp4 --gif
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from rhc_demo.main_window import MainWindow


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="rhc-demo", description="Dynamics-morphing standing/hopping demo.")
    sub = parser.add_subparsers(dest="mode")

    sub.add_parser("interactive", help="live simulation window (default)")

    replay = sub.add_parser("replay", help="replay a recorded time-series CSV")
    replay.add_argument("csv", type=Path, help="logger-schema CSV file")
    replay.add_argument("--speed", type=float, default=1.0, help="playback speed factor")

    headless = sub.add_parser("headless", help="render a CSV offscreen to frames and video")
    headless.add_argument("csv", type=Path, help="logger-schema CSV file")
    headless.add_argument("-o", "--out", type=Path, default=Path("rhc_demo_out"), help="output directory")
    headless.add_argument("--fps", type=int, default=50, help="output framerate")
    headless.add_argument("--speed", type=float, default=1.0, help="playback speed factor")
    headless.add_argument("--size", default="1280x720", help="frame size WxH")
    headless.add_argument("--mp4", action="store_true", help="also encode an MP4")
    headless.add_argument("--gif", action="store_true", help="also encode a GIF")
    return parser


def _run_gui(window_factory) -> int:
    from PyQt6.QtWidgets import QApplication

    app = QApplication(sys.argv)
    window = window_factory()
    window.resize(1280, 720)
    window.show()
    return app.exec()


def _main_interactive() -> int:
    def factory() -> MainWindow:
        from rhc_demo.engine import LiveEngine
        from rhc_demo.main_window import MainWindow

        return MainWindow(LiveEngine())

    return _run_gui(factory)


def _main_replay(csv: Path, speed: float) -> int:
    def factory() -> MainWindow:
        from rhc_demo.main_window import MainWindow
        from rhc_demo.replay import load_replay

        return MainWindow(load_replay(csv, speed=speed))

    return _run_gui(factory)


def _main_headless(args: argparse.Namespace) -> int:
    # Offscreen rendering is mandatory (Wayland restricts window capture);
    # the platform must be fixed before Qt is imported/instantiated.
    os.environ["QT_QPA_PLATFORM"] = "offscreen"
    from PyQt6.QtWidgets import QApplication

    from rhc_demo.headless import encode_gif, encode_mp4, render_frames
    from rhc_demo.replay import load_replay

    try:
        width, height = (int(v) for v in args.size.lower().split("x"))
    except ValueError:
        print(f"invalid --size {args.size!r}, expected WxH like 1280x720", file=sys.stderr)
        return 2

    _app = QApplication([])  # must outlive all widgets created below
    source = load_replay(args.csv, speed=args.speed)
    frame_dir = args.out / "frames"

    def progress(done: int, total: int) -> None:
        if done % 25 == 0 or done == total:
            print(f"\rrendering frames: {done}/{total}", end="", flush=True)

    frames = render_frames(source, frame_dir, fps=args.fps, size=(width, height), speed=args.speed, progress=progress)
    print(f"\n{len(frames)} frames -> {frame_dir}")

    if args.mp4:
        out = encode_mp4(frame_dir, args.out / f"{args.csv.stem}.mp4", fps=args.fps)
        print(f"encoded {out}")
    if args.gif:
        out = encode_gif(frame_dir, args.out / f"{args.csv.stem}.gif", fps=args.fps)
        print(f"encoded {out}")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    if args.mode == "replay":
        return _main_replay(args.csv, args.speed)
    if args.mode == "headless":
        return _main_headless(args)
    return _main_interactive()


if __name__ == "__main__":
    sys.exit(main())
