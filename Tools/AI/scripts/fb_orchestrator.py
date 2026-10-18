"""Entry point. Adds the automation package to sys.path and runs the CLI."""

from __future__ import annotations

import sys
from pathlib import Path

AI_ROOT = Path(__file__).resolve().parents[1]
if str(AI_ROOT) not in sys.path:
    sys.path.insert(0, str(AI_ROOT))

from fb_auto.cli import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
