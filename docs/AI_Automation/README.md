# Final Bell automation

Ask in normal language. The toolchain picks a workflow, runs the local steps it can, and says when a person still has to open another application.

The orchestrator does not replace the boxing ring, the broadcast camera, or the fighter controls.

## Commands

From the repo root, with real Python 3.11+ (not the Microsoft Store alias):

```text
python Tools/AI/scripts/fb_orchestrator.py route "Create a lightweight boxer with fast hands."
python Tools/AI/scripts/fb_orchestrator.py run create-boxer --request "Create a lightweight boxer with fast hands."
python Tools/AI/scripts/fb_orchestrator.py status
python Tools/AI/scripts/fb_orchestrator.py check --write-report
```

PowerShell wrapper, if `python` still points at the Store stub:

```text
powershell -ExecutionPolicy Bypass -File Tools/AI/scripts/Invoke-FinalBellWorkflow.ps1 route "Create a lightweight boxer with fast hands."
```

Install or re-check free tools:

```text
powershell -ExecutionPolicy Bypass -File Tools/Setup/Install-FinalBell-Toolchain.ps1
powershell -ExecutionPolicy Bypass -File Tools/Setup/Install-FinalBell-Toolchain.ps1 -AuditOnly
```

## What to say

| Request | What runs |
| --- | --- |
| Create a lightweight boxer with fast hands. | Fighter JSON for a lightweight fast-hands build. Optional Blender scale proxy with `--blender`. |
| Create three realistic heavyweight fighters... | Three heavyweight specs with different height, skin, and face seeds. |
| Add realistic counterpunching. | Points at the existing combat C++. It does not rewrite combat by itself. |
| Improve character rotation and footwork. | Points at `UpdateFacingOffset` and movement input in `FBBoxerCharacter`. |
| Generate a career-mode menu. | Layout JSON from the local design system. Figma is not called. |
| Add facial damage after punches. | Texture handoff manifest. Cosmetic masks stay separate from server damage. |
| Create a realistic knockout replay. | Animation manifest and the expected clip names. It does not invent a replay. |

## Where files go

- Fighter specs: `Tools/AI/output/fighters/`
- Scale proxies: `Tools/AI/output/fighters/proxies/`
- Animation inbox: `Tools/Animation/incoming/`
- Adobe exports: `Tools/Adobe/inbox/`
- UI specs: `Tools/AI/output/ui/`
- Logs: `Tools/AI/logs/orchestrator.jsonl`

Generated files are kept out of `apps/game-unreal/Content` and `Source`. A spec with status `generated_unvalidated_for_production` is not a finished fighter.

## Applications

MetaHuman Character is installed with Unreal Engine 5.8 and left disabled in `FinalBell.uproject`. Enable it in the editor when you want to author a face. It does not include boxing clothes, gloves, or the current punch poses.

Blender 4.5 LTS is the scripted mesh tool. `Tools/Blender/fb_export_scale_reference.py` exports a labeled cube. `fb_cleanup_animation.py` and `fb_validate_mesh.py` report armature, UV, and polygon facts. They do not mark a file production-ready.

DeepMotion and Rokoko are not installed. Export FBX into the animation inbox. The Rokoko plugin was not in the engine plugin directory.

Figma, Linear, Adobe, and Hugging Face accounts are not connected. Set `FIGMA_ACCESS_TOKEN`, `LINEAR_API_KEY`, or `HF_TOKEN` locally if you want those calls. Model search uses the public API and does not download weights. Linear issues are listed locally and are not created.

## Checks

```text
python -m unittest discover -s Tools/AI/tests -v
python Tools/AI/scripts/fb_orchestrator.py check --write-report
```

The check writes `docs/AI_Automation/Integration_Test_Report.md` from the process it just ran. It does not compile Unreal and it does not play a match.

GitHub Actions, in `.github/workflows/validate.yml`, runs the same unit tests, the invariant check, and the golden contract tests. It does not push assets.

## More detail

- Environment facts: `docs/AI_Automation/Environment_Audit.md`
- Tool list: `docs/AI_Automation/Tool_Inventory.md`
- Two-person Git practice: `docs/AI_Automation/Collaboration.md`
- Steps that still need you: `docs/AI_Automation/Manual_Steps.md`
- Cursor prompts: `Tools/AI/prompts/`
