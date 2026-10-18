# Environment audit

Audit date: 2026-10-18. Values come from this machine and this repo. The boxing ring, fighter controls, and broadcast camera were left in place.

## Machine

| Item | Observed |
| --- | --- |
| OS | Microsoft Windows 11 Home, version 10.0.26300, 64-bit |
| CPU | AMD EPYC 7543P, 4 cores and 8 logical processors visible, 2800 MHz |
| GPU | NVIDIA RTX A4500, 19190 MiB, driver 565.90. A Red Hat QXL display adapter is also present. |
| RAM | 28 GB visible, about 19.7 GB free at audit time |
| Disk | C: 510.5 GB, 419.7 GB free |

## Unreal project

| Item | Observed |
| --- | --- |
| Project | `apps/game-unreal/FinalBell.uproject` |
| Engine association | 5.8 |
| Installed editor | `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` |
| Engine build | 5.8.3, changelist 58210709, branch `++UE5+Release-5.8` |
| Module | `FinalBell` runtime module |
| Enabled plugins | EnhancedInput, GameInput, GameInputWindows, GameplayAbilities, PythonScriptPlugin, ModelingToolsEditorMode |
| Project plugins folder | None |
| Build tool | `Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe` is present. This session did not run an editor compile. |
| C++ toolset | Visual Studio 2022 Build Tools 17.14.42, MSVC 14.44.35207, Windows SDK 10.0.26100.0 and 10.0.19041.0 |
| Compiler | `cl.exe` is installed and is off the default PATH, which is normal until a developer command prompt is used |

A registry key `HKLM\SOFTWARE\EpicGames\Unreal Engine\4.0` points at `C:\Program Files\Epic Games\4.0\`. The Epic Games directory listing showed `UE_5.8`, the launcher, and redistributables. The live editor is 5.8.3.

MetaHuman is installed with the engine under `Engine\Plugins\MetaHuman`, including `MetaHumanCharacter`. It is not enabled in `FinalBell.uproject`. No Rokoko plugin was found in the engine plugins tree.

## Gameplay already in the project

| System | Implementation |
| --- | --- |
| Ring | `UFBP1ArenaBootstrap::EnsureArena` builds the floor, lights, bounds, opponent, and camera. `AFBRingBounds` clamps the playable area. |
| Movement and rotation | `AFBBoxerCharacter` faces the opponent in `UpdateFacingOffset`, with `FaceTurnSpeed`, `MaxFacingOffsetDegrees`, and stick movement relative to actor yaw. |
| Punches | Jab, cross, lead hook, rear hook, lead uppercut, and rear uppercut. The server owns `ServerSubmitCombatIntent`. |
| Camera | `AFBBroadcastCamera` frames both fighters from ringside. |
| Map | `Content/Maps/Ring_P1.umap` is tracked with Git LFS. |
| Content | `Content/Mannequins` and `Content/Maps` exist. The mannequin files are untracked. |

`scripts/create_ring_map.py` deletes `/Game/Maps/Ring_P1` before recreating it. It was not run.

## Git

| Item | Observed |
| --- | --- |
| Repository | `C:\Users\Shadow\Projects\final-bell` |
| Remote | `https://github.com/SportsGamer2010/finalbell.git` |
| Current branch | `cursor/p1-visible-arena-dualsense-input` |
| Other local branch | `master`, tracking `origin/master` |
| Git | 2.55.0.windows.5 |
| Git LFS | 3.7.1. One LFS file: `apps/game-unreal/Content/Maps/Ring_P1.umap`. Zero tracked `.uasset` files. |
| GitHub CLI | 2.102.0 installed. `gh auth status` reports that no host is logged in. |

Nothing was pushed. Repository visibility was not changed.

## Tools after setup

| Tool | Result |
| --- | --- |
| Git | Already installed |
| GitHub CLI 2.102.0 | Installed with winget, user scope |
| Python 3.12.10 | Installed with winget, user scope. The previous `python` command was the Microsoft Store alias. |
| Blender 4.5.10 LTS | Official zip from `download.blender.org`, extracted to `%LOCALAPPDATA%\Programs\Blender Foundation\Blender 4.5`. The `.sha256` sidecar URL returned 404, so the archive hash was not checked. `blender --version` printed 4.5.10 LTS. Winget's Blender installer asked for administrator approval and was not used. |
| FFmpeg 9.0.2 | Installed with winget, user scope |
| Node.js 22.23.2 | Installed with winget, user scope |
| Visual Studio C++ | Already installed. The large Build Tools installer was not started. |
| Unreal Engine | Existing 5.8.3 kept |
| Cursor | 3.24.18 already installed |
| .NET SDK on PATH | `dotnet` was not on PATH. Unreal's own UnrealBuildTool executable is present. |

Python packages installed for that interpreter: jsonschema 4.23.0, pytest 8.3.5, Pillow 11.1.0.

A second installer run skipped the tools that were already present and reported failed=0. `-AuditOnly` then listed Git, Git LFS, Unreal, MSVC, Cursor, GitHub CLI, Python, Blender, FFmpeg, and Node as skipped, with failed=0.

## Integrations

No `.cursor/mcp.json` exists in the project or the user Cursor config that was checked. Figma, Linear, Adobe, DeepMotion, and Rokoko credentials are unset. Hugging Face public search works without a token. An example MCP file is at `Tools/AI/config/mcp.example.json` and was not activated.

## Sample workflow that ran

`create-boxer` wrote fighter JSON under `Tools/AI/output/fighters/`. Three heavyweight specs from seed 7:

| Id | Name | Height | Reach | Build | Melanin |
| --- | --- | --- | --- | --- | --- |
| fb_hw_0007_00 | Caleb Moreau | 185 cm | 200.5 cm | thick | 0.12 |
| fb_hw_0007_01 | Seth Pell | 193 cm | 213 cm | long-limbed | 0.40 |
| fb_hw_0007_02 | Milo Delgado | 201 cm | 188 cm | athletic | 0.68 |

Blender exported `fb_lw_0042_00_scale_proxy.fbx` (11,788 bytes). The mesh check reported `FB_GEN_Proxy_Body`, 6 polygons, 1 UV layer, and `production_ready` false. These files are outside `Content/`.
