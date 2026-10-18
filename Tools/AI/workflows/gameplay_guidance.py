"""Point a gameplay request at the existing C++ systems. Do not edit them from here."""

from __future__ import annotations

import json

from fb_auto.paths import CONFIG


def guide(request: str) -> dict:
    points = json.loads((CONFIG / "combat_extension_points.json").read_text(encoding="utf-8"))
    return {
        "status": "guidance",
        "executed": False,
        "code_changed": False,
        "request": request,
        "inspect": points,
        "manual": "Edit the listed C++ only after reading it. Run a real FinalBellEditor build before calling the change done.",
    }
