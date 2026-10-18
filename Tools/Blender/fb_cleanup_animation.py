"""Inspect an FBX or blend file for an armature and print a JSON report.

  blender --background --python Tools/Blender/fb_cleanup_animation.py -- --input clip.fbx
"""

from __future__ import annotations

import json
import sys


def _args(argv: list[str]) -> dict[str, str]:
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    values: dict[str, str] = {}
    index = 0
    while index < len(argv):
        if argv[index].startswith("--") and index + 1 < len(argv):
            values[argv[index][2:]] = argv[index + 1]
            index += 2
        else:
            index += 1
    return values


def main() -> int:
    import bpy

    options = _args(sys.argv)
    source = options["input"]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if source.lower().endswith(".fbx"):
        bpy.ops.import_scene.fbx(filepath=source)
    elif source.lower().endswith(".blend"):
        bpy.ops.wm.open_mainfile(filepath=source)
    else:
        print(json.dumps({"ok": False, "error": "expected .fbx or .blend"}))
        return 1
    armatures = [obj.name for obj in bpy.data.objects if obj.type == "ARMATURE"]
    actions = [action.name for action in bpy.data.actions]
    report = {
        "ok": bool(armatures),
        "armatures": armatures,
        "actions": actions,
        "fps": bpy.context.scene.render.fps,
        "production_ready": False,
        "review": "Confirm foot contact and retarget onto SK_Mannequin.",
    }
    print("FB_ANIM_REPORT " + json.dumps(report))
    return 0 if armatures else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ModuleNotFoundError:
        sys.stderr.write("This script must be launched by Blender so bpy is available.\n")
        raise SystemExit(2)
