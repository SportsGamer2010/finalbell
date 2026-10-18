# New boxer

1. Run `python Tools/AI/scripts/fb_orchestrator.py route` on the request.
2. Run `create-boxer` with the extracted weight, style, and count.
3. Validate every spec. Hitboxes stay on the canonical class capsule.
4. If Blender is installed and a scale check is useful, pass `--blender`. The FBX is a proxy named `FB_GEN_Proxy_Body`.
5. Stop before MetaHuman. Tell the user the plugin is installed with UE 5.8 and still disabled in the project.
6. Do not copy the spec or proxy into `Content/`.
