"""Append-only JSONL logs for workflow runs."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import Any

from fb_auto.paths import LOGS
from fb_auto.safety import assert_safe_output


def log_event(workflow: str, status: str, detail: dict[str, Any]) -> None:
    LOGS.mkdir(parents=True, exist_ok=True)
    record = {
        "time": datetime.now(timezone.utc).isoformat(),
        "workflow": workflow,
        "status": status,
        "detail": detail,
    }
    path = assert_safe_output(LOGS / "orchestrator.jsonl")
    with path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(record, sort_keys=True) + "\n")
