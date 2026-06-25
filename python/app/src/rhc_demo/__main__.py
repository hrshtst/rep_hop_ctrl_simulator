"""Entry point for the rhc-demo application."""

from __future__ import annotations

import sys


def main() -> None:
    """Launch the dynamics-morphing hopping demo window."""
    from PyQt6.QtWidgets import QApplication

    from rhc_demo.main_window import MainWindow

    app = QApplication(sys.argv)
    window = MainWindow()
    window.resize(1150, 620)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
