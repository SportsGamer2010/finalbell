"""Export a labeled scale-reference cube. Not a production boxer.

Run from Blender:
  blender --background --python Tools/Blender/fb_export_scale_reference.py -- --spec spec.json --out proxy.fbx
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
        key = argv[index]
        if key.startswith("--") and index + 1 < len(argv):
            values[key[2:]] = argv[index + 1]
            index += 2
        else:
            index += 1
    return values


def main() -> int:
    import bpy

    options = _args(sys.argv)
    spec = json.loads(open(options["spec"], encoding="utf-8").read())
    height = float(spec["presentation"]["height_cm"])
    shoulder = float(spec["presentation"]["shoulder_width_cm"])
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, height / 2.0))
    body = bpy.context.active_object
    body.name = "FB_GEN_Proxy_Body"
    body.scale = (shoulder / 2.0, 28.0, height / 2.0)
    bpy.ops.object.transform_apply(scale=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 0.01
    bpy.ops.export_scene.fbx(
        filepath=options["out"],
        use_selection=False,
        apply_unit_scale=True,
        object_types={"MESH"},
    )
    print(f"FB_PROXY_EXPORTED {options['out']}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ModuleNotFoundError:
        sys.stderr.write("This script must be launched by Blender so bpy is available.\n")
        raise SystemExit(2)
