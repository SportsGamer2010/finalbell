"""Check Final Bell animation clip names."""

from __future__ import annotations

import json
import re

from fb_auto.paths import CONFIG


def _catalog() -> dict:
    return json.loads((CONFIG / "animation_names.json").read_text(encoding="utf-8"))


def validate_clip_name(name: str) -> list[str]:
    catalog = _catalog()
    errors: list[str] = []
    match = re.fullmatch(r"FB_(Shared|LW|MW|HW)_(Move|Punch|Defense|React)_([A-Za-z0-9]+)_(\d{2})", name)
    if not match:
        return ["name must look like FB_Shared_Punch_Jab_01"]
    category, action = match.group(2), match.group(3)
    allowed = catalog["categories"].get(category, [])
    if action not in allowed:
        errors.append(f"{action} is not in the {category} vocabulary")
    return errors


def expected_clips() -> list[str]:
    return list(_catalog()["priority"])
