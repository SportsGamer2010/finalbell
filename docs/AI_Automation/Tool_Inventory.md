# Tool and integration inventory

Status after the 2026-10-18 setup run. Authentication says whether a call can be made today.

| Tool | Version or location | Workflow | Authentication |
| --- | --- | --- | --- |
| Git | 2.55.0.windows.5 | Version control | Local repo. Remote is GitHub. |
| Git LFS | 3.7.1 | `.uasset`, `.umap`, `.fbx`, images, audio, `.blend`, `.bvh` | Local hooks installed |
| GitHub CLI | 2.102.0 | Pull requests and review | Not logged in |
| Python | 3.12.10 | Orchestrator, contracts, textures | Local |
| jsonschema, pytest, Pillow | 4.23.0, 8.3.5, 11.1.0 | Validation | Installed for the user |
| Blender | 4.5.10 LTS | Scale proxy, mesh check, animation inspect | Local executable |
| FFmpeg | 9.0.2 full build | Reference video | Local |
| Node.js | 22.23.2 | Future MCP or API helpers | Local. No MCP server is configured. |
| Unreal Engine | 5.8.3 | Game project | Installed editor |
| MSVC | 14.44.35207 with VS 2022 Build Tools 17.14.42 | C++ compile | Installed. A compile was not run. |
| Cursor | 3.24.18 | Editing and tool routing | This session |
| MetaHuman Character | Engine plugin present | Face authoring | Disabled in `FinalBell.uproject` |
| Figma | No API call | UI spec is local | `FIGMA_ACCESS_TOKEN` unset |
| Linear | Local workstream file, 10 tasks | Planning | `LINEAR_API_KEY` unset. No issues created. |
| Hugging Face | Public model search, HTTP 200 for "pose estimation" | Research | No token. No weights downloaded. |
| Adobe | Folder inbox `Tools/Adobe/inbox` | Texture manifest | No API |
| DeepMotion | FBX or BVH inbox | Animation names | Not installed |
| Rokoko | FBX inbox | Animation names | Engine plugin not present |

The live registry is `Tools/AI/config/workflow_registry.json`. Refresh the machine view with:

```text
python Tools/AI/scripts/fb_orchestrator.py status
```
