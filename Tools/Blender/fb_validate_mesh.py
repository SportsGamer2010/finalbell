"""Report mesh name, polygon count, and UV presence for an FBX.

  blender --background --python Tools/Blender/fb_validate_mesh.py -- --input mesh.fbx
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

    source = _args(sys.argv)["input"]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=source)
    meshes = []
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        meshes.append(
            {
                "name": obj.name,
                "polygons": len(obj.data.polygons),
                "uv_layers": len(obj.data.uv_layers),
                "generated_prefix": obj.name.startswith("FB_GEN_"),
            }
        )
    report = {"ok": bool(meshes), "meshes": meshes, "production_ready": False}
    print("FB_MESH_REPORT " + json.dumps(report))
    return 0 if meshes else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ModuleNotFoundError:
        sys.stderr.write("This script must be launched by Blender so bpy is available.\n")
        raise SystemExit(2)
