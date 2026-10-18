"""Map a natural-language request to a workflow. Routing does not execute the workflow."""

from __future__ import annotations

import json
import re
from typing import Any

from fb_auto.paths import CONFIG

WEIGHTS = {
    "lightweight": "lightweight",
    "middleweight": "middleweight",
    "heavyweight": "heavyweight",
}
STYLES = (
    "fast-hands",
    "fast hands",
    "out-boxer",
    "pressure",
    "counterpuncher",
    "counterpunching",
    "slugger",
    "boxer-puncher",
)
COUNTS = {
    "one": 1,
    "two": 2,
    "three": 3,
    "four": 4,
}


def load_registry() -> dict[str, Any]:
    return json.loads((CONFIG / "workflow_registry.json").read_text(encoding="utf-8"))


def route(request: str) -> dict[str, Any]:
    registry = load_registry()
    text = request.lower()
    scored: list[dict[str, Any]] = []
    for entry in registry["routes"]:
        hits = [pattern for pattern in entry["patterns"] if re.search(pattern, text)]
        if hits:
            scored.append({**entry, "score": len(hits), "hits": hits})
    scored.sort(key=lambda item: item["score"], reverse=True)
    primary = scored[0]["id"] if scored else "gameplay"
    return {
        "request": request,
        "primary": primary,
        "matches": [
            {"id": item["id"], "title": item["title"], "score": item["score"], "tools": item["tools"]}
            for item in scored
        ],
        "extracted": extract_args(request),
        "executed": False,
    }


def extract_args(request: str) -> dict[str, Any]:
    text = request.lower()
    weight = next((value for key, value in WEIGHTS.items() if key in text), None)
    style = None
    for name in STYLES:
        if name in text:
            style = "fast-hands" if name == "fast hands" else name
            if style == "counterpunching":
                style = "counterpuncher"
            break
    count = 1
    number = re.search(r"\b(\d+)\b", text)
    if number:
        count = max(1, min(8, int(number.group(1))))
    else:
        for word, value in COUNTS.items():
            if re.search(rf"\b{word}\b", text):
                count = value
                break
    screen = None
    screens = json.loads((CONFIG / "ui_screens.json").read_text(encoding="utf-8"))["screens"]
    for name in screens:
        if name.replace("-", " ") in text:
            screen = name
            break
    if screen is None and "career" in text and "menu" in text:
        screen = "career-mode"
    if screen is None and "menu" in text:
        screen = "main-menu"
    return {"weight": weight, "style": style, "count": count, "screen": screen}
