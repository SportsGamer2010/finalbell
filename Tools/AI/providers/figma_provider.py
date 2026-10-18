"""Report Figma auth state. No file is fetched unless a later command adds a file key."""

from __future__ import annotations

import os


def status() -> dict:
    configured = bool(os.environ.get("FIGMA_ACCESS_TOKEN") or os.environ.get("FIGMA_TOKEN"))
    return {
        "configured": configured,
        "api_called": False,
        "mcp": False,
        "manual": "Set FIGMA_ACCESS_TOKEN and a file key before requesting a Figma export.",
    }
