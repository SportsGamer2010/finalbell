"""Scan the Adobe handoff inbox and write an Unreal-oriented texture manifest."""

from __future__ import annotations

import json

from fb_auto.logging_util import log_event
from fb_auto.paths import ADOBE_INBOX, ADOBE_OUTBOX, ensure_output
from fb_auto.safety import assert_safe_output

RUNTIME_OK = {".png", ".jpg", ".jpeg", ".tga", ".tif", ".tiff"}
NEEDS_EXPORT = {".psd", ".psb"}


def build_texture_manifest() -> dict:
    ADOBE_INBOX.mkdir(parents=True, exist_ok=True)
    ADOBE_OUTBOX.mkdir(parents=True, exist_ok=True)
    entries = []
    for path in sorted(ADOBE_INBOX.iterdir()):
        if not path.is_file() or path.name == ".gitkeep":
            continue
        suffix = path.suffix.lower()
        entry = {
            "file": path.name,
            "bytes": path.stat().st_size,
            "unreal_runtime_format": suffix in RUNTIME_OK,
            "action": "export to PNG or TGA" if suffix in NEEDS_EXPORT else "review resolution",
        }
        if suffix in RUNTIME_OK:
            entry.update(_image_facts(path))
        entries.append(entry)
    manifest = {
        "status": "completed",
        "executed": True,
        "adobe_api_called": False,
        "files": entries,
        "material_parameters": {
            "SweatAmount": "cosmetic",
            "BruiseMask": "cosmetic",
            "Swelling": "cosmetic",
            "CutMask": "cosmetic",
            "authoritative_damage": "owned by the combat server, not by these textures",
        },
    }
    destination = assert_safe_output(ensure_output("textures") / "texture_manifest.json")
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    outbox = assert_safe_output(ADOBE_OUTBOX / "texture_manifest.json")
    outbox.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    log_event("textures", "completed", {"count": len(entries), "manifest": str(destination)})
    manifest["manifest_path"] = str(destination)
    return manifest


def _image_facts(path) -> dict:
    try:
        from PIL import Image
    except ImportError:
        return {"pixel_check": "skipped: Pillow is not installed"}
    with Image.open(path) as image:
        width, height = image.size
    return {
        "width": width,
        "height": height,
        "multiple_of_four": width % 4 == 0 and height % 4 == 0,
    }
