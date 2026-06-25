from __future__ import annotations

import os

# Run Qt without a display for the GUI smoke tests.
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
