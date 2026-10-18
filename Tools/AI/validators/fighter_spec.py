"""Validate generated fighter specs. Appearance must not become the hitbox."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from fb_auto.paths import CONFIG


def _load_classes() -> dict[str, Any]:
    return json.loads((CONFIG / "weight_classes.json").read_text(encoding="utf-8"))


def validate_fighter(spec: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    classes = _load_classes()
    if spec.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    status = str(spec.get("asset_status", ""))
    if not status.startswith(("generated_", "approved_")):
        errors.append("asset_status must start with generated_ or approved_")
    weight = spec.get("weight_class")
    band = classes["classes"].get(weight)
    if band is None:
        errors.append("weight_class must be lightweight, middleweight, or heavyweight")
        return errors
    gameplay = spec.get("gameplay") or {}
    if gameplay.get("hitbox_policy") != "canonical_class_capsule":
        errors.append("hitbox_policy must stay canonical_class_capsule")
    if gameplay.get("hitboxes_follow_appearance") is not False:
        errors.append("hitboxes_follow_appearance must be false")
    cosmetic = spec.get("cosmetic_damage") or {}
    if cosmetic.get("separate_from_authoritative_state") is not True:
        errors.append("cosmetic damage must stay separate from authoritative state")
    presentation = spec.get("presentation") or {}
    height = presentation.get("height_cm")
    reach = presentation.get("reach_cm")
    if not _in_band(height, band["height_cm"], 2):
        errors.append("height_cm is outside the class band")
    if not _in_band(reach, band["reach_cm"], 2):
        errors.append("reach_cm is outside the class band")
    expected = round(float(height) / classes["reference_height_cm"], 4) if height else None
    scale = presentation.get("body_scale_on_180cm_mannequin")
    if expected is None or scale is None or abs(float(scale) - expected) > 0.002:
        errors.append("body scale must equal height divided by 180")
    if presentation.get("body_type") not in band["body_types"]:
        errors.append("body_type is not valid for the class")
    return errors


def validate_file(path: Path) -> list[str]:
    spec = json.loads(path.read_text(encoding="utf-8"))
    return validate_fighter(spec)


def _in_band(value: Any, band: list[float], slack: float) -> bool:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return False
    return (band[0] - slack) <= number <= (band[1] + slack)
