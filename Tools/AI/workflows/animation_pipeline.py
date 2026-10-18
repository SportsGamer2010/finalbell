"""Name-check motion files and write the clip list the project still needs."""

from __future__ import annotations

import json
from pathlib import Path

from fb_auto.logging_util import log_event
from fb_auto.paths import ANIM_INCOMING, ensure_output
from fb_auto.safety import assert_safe_output
from validators.animation_names import expected_clips, validate_clip_name


def build_manifest(incoming: Path | None = None) -> dict:
    folder = incoming or ANIM_INCOMING
    folder.mkdir(parents=True, exist_ok=True)
    reports = []
    for path in sorted(folder.iterdir()):
        if not path.is_file() or path.name == ".gitkeep":
            continue
        stem = path.stem
        reports.append(
            {
                "file": str(path),
                "errors": validate_clip_name(stem),
                "production_ready": False,
                "review": "Check foot contact, weight transfer, and retarget to SK_Mannequin before use.",
            }
        )
    manifest = {
        "status": "completed",
        "executed": True,
        "incoming_count": len(reports),
        "files": reports,
        "expected": expected_clips(),
        "sources": {
            "deepmotion": "not_called",
            "rokoko": "not_called",
            "note": "Drop FBX or BVH into Tools/Animation/incoming. This manifest does not generate motion.",
        },
    }
    destination = assert_safe_output(ensure_output("animation") / "animation_manifest.json")
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    log_event("animation", "completed", {"manifest": str(destination), "incoming": len(reports)})
    manifest["manifest_path"] = str(destination)
    return manifest
