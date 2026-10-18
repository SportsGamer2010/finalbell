"""Build fighter specification files and an optional Blender scale proxy.

The proxy is a generated preview. It is not a MetaHuman and it is not imported
into the Unreal content folder.
"""

from __future__ import annotations

import json
import random
import subprocess
from pathlib import Path
from typing import Any

from fb_auto.environment import probe
from fb_auto.logging_util import log_event
from fb_auto.paths import CONFIG, REPO_ROOT, ensure_output
from fb_auto.safety import assert_safe_output
from validators.fighter_spec import validate_fighter

FIRST = (
    "Milo", "Andre", "Caleb", "Darius", "Evan", "Felix", "Jonah", "Malik",
    "Nico", "Omar", "Rafael", "Seth", "Tomas", "Victor", "Wesley",
)
LAST = (
    "Carver", "Delgado", "Holtz", "Ibarra", "Keene", "Lang", "Moreau",
    "Navarro", "Okonkwo", "Pell", "Quint", "Sato", "Voss", "Ward", "Yeung",
)
HAIR = ("bald", "crop", "short", "caesar", "fade")
FACIAL_HAIR = ("none", "mustache", "short-beard", "goatee")
UNDERTONES = ("neutral", "warm", "cool")
CORNERS = ("#1F4B99", "#8E2A2A", "#D6A03C", "#F4F1EA")


def build_fighter(weight: str, style: str, seed: int, index: int = 0) -> dict[str, Any]:
    classes = json.loads((CONFIG / "weight_classes.json").read_text(encoding="utf-8"))
    band = classes["classes"][weight]
    style_stats = classes["styles"][style]
    rng = random.Random(seed + index * 997)
    height = _spread(rng, band["height_cm"], index)
    reach = _spread(rng, band["reach_cm"], index + 1)
    shoulder = _spread(rng, band["shoulder_width_cm"], index + 2)
    muscle = _spread(rng, band["muscle_mass"], index + 3)
    body_type = band["body_types"][index % len(band["body_types"])]
    code = band["code"]
    fighter_id = f"fb_{code}_{seed:04d}_{index:02d}"
    spec = {
        "schema_version": 1,
        "asset_status": "generated_unvalidated_for_production",
        "id": fighter_id,
        "display_name": f"{rng.choice(FIRST)} {rng.choice(LAST)}",
        "weight_class": weight,
        "style": style,
        "stance": "southpaw" if index % 2 else "orthodox",
        "presentation": {
            "height_cm": round(height, 1),
            "reach_cm": round(reach, 1),
            "shoulder_width_cm": round(shoulder, 1),
            "muscle_mass": round(muscle, 3),
            "body_type": body_type,
            "body_scale_on_180cm_mannequin": round(height / classes["reference_height_cm"], 4),
            "skin": {
                "melanin": round(0.12 + (index * 0.28) % 0.8, 3),
                "undertone": UNDERTONES[index % len(UNDERTONES)],
            },
            "face": {
                "seed": seed + index,
                "brow": round(rng.random(), 3),
                "jaw": round(rng.random(), 3),
                "nose": round(rng.random(), 3),
            },
            "hair": HAIR[index % len(HAIR)],
            "facial_hair": FACIAL_HAIR[(index + seed) % len(FACIAL_HAIR)],
        },
        "equipment": {
            "gloves": {"color": CORNERS[index % len(CORNERS)], "oz": band["glove_oz"]},
            "trunks": {
                "color": CORNERS[index % len(CORNERS)],
                "stripe": CORNERS[(index + 1) % len(CORNERS)],
            },
        },
        "cosmetic_damage": {
            "separate_from_authoritative_state": True,
            "sweat": 0,
            "bruise": 0,
            "swelling": 0,
            "cuts": 0,
            "material_parameters": ["SweatAmount", "BruiseMask", "Swelling", "CutMask"],
        },
        "gameplay": {
            "hitbox_policy": "canonical_class_capsule",
            "hitboxes_follow_appearance": False,
            **style_stats,
        },
        "sources": {
            "metahuman": "not_generated",
            "blender_proxy": None,
            "note": "Specification only. A MetaHuman face and boxing wardrobe still have to be authored.",
        },
    }
    errors = validate_fighter(spec)
    if errors:
        raise RuntimeError("Fighter generator produced an invalid spec: " + "; ".join(errors))
    return spec


def write_fighters(
    weight: str,
    style: str,
    count: int,
    seed: int,
    use_blender: bool,
) -> dict[str, Any]:
    folder = ensure_output("fighters")
    written: list[str] = []
    specs: list[dict[str, Any]] = []
    for index in range(count):
        spec = build_fighter(weight, style, seed, index)
        path = assert_safe_output(folder / f"{spec['id']}.json")
        path.write_text(json.dumps(spec, indent=2) + "\n", encoding="utf-8")
        written.append(str(path))
        specs.append(spec)
    proxy = None
    blender_status = "not_requested"
    if use_blender:
        proxy, blender_status = _export_proxy(specs[0])
        if proxy:
            specs[0]["sources"]["blender_proxy"] = proxy
            first = Path(written[0])
            first.write_text(json.dumps(specs[0], indent=2) + "\n", encoding="utf-8")
    result = {
        "status": "completed",
        "executed": True,
        "files": written,
        "blender": blender_status,
        "proxy": proxy,
        "production_ready": False,
        "manual": [
            "Enable MetaHumanCharacter in the UE 5.8 editor when a real face is required.",
            "Do not import the scale proxy over the mannequin used by the live ring.",
        ],
    }
    log_event("create-boxer", "completed", {"files": written, "blender": blender_status})
    return result


def _export_proxy(spec: dict[str, Any]) -> tuple[str | None, str]:
    blender = probe()["tools"]["blender"]
    if not blender.get("installed"):
        log_event("create-boxer", "manual", {"reason": "Blender is not installed"})
        return None, "manual: Blender is not installed"
    script = REPO_ROOT / "Tools" / "Blender" / "fb_export_scale_reference.py"
    destination = assert_safe_output(
        ensure_output("fighters", "proxies") / f"{spec['id']}_scale_proxy.fbx"
    )
    spec_path = assert_safe_output(ensure_output("fighters") / f"{spec['id']}.json")
    completed = subprocess.run(
        [
            blender["path"],
            "--background",
            "--python",
            str(script),
            "--",
            "--spec",
            str(spec_path),
            "--out",
            str(destination),
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=180,
    )
    log_path = assert_safe_output(ensure_output("fighters", "proxies") / f"{spec['id']}_blender.log")
    log_path.write_text(completed.stdout + "\n" + completed.stderr, encoding="utf-8")
    if completed.returncode != 0 or not destination.is_file():
        log_event("create-boxer", "failed", {"returncode": completed.returncode, "log": str(log_path)})
        return None, f"failed: see {log_path}"
    return str(destination), "completed"


def _spread(rng: random.Random, band: list[float], index: int) -> float:
    if index % 3 == 0:
        return band[0]
    if index % 3 == 1:
        return (band[0] + band[1]) / 2
    return band[1] if rng.random() > 0.15 else band[0] + (band[1] - band[0]) * 0.72
