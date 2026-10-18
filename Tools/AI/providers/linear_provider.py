"""Read the local Linear plan. Creating issues requires an API key and is not run here."""

from __future__ import annotations

import json
import os

from fb_auto.paths import CONFIG


def plan() -> dict:
    data = json.loads((CONFIG / "linear_workstreams.json").read_text(encoding="utf-8"))
    configured = bool(os.environ.get("LINEAR_API_KEY"))
    return {
        "status": "planned" if not configured else "configured",
        "executed": False,
        "api_called": False,
        "configured": configured,
        "workstreams": [item["name"] for item in data["workstreams"]],
        "task_count": sum(len(item["tasks"]) for item in data["workstreams"]),
        "manual": data["apply"],
    }
