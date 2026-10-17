# Final Bell

Gameplay-first boxing simulation (Unreal Engine 5 C++ + Python domain/contracts).

| Item | Value |
| --- | --- |
| Display name | Final Bell |
| Unreal project / module | `FinalBell` |
| Repository slug | `final-bell` |
| Python prefix | `final_bell` |
| Engine target | UE 5.8 (installed locally at audit time) |

## Layout

```
apps/game-unreal/     Unreal C++ game (combat authority, presentation hooks)
packages/contracts/   Versioned JSON schemas + fixtures
packages/domain-python/  Pure world/ranking/reference math (no combat frame loop)
docs/                 Architecture, progress, backlog, decisions
tests/golden/         Cross-language golden fixtures
```

## Toolchain status (P0 audit)

See [docs/Progress.md](docs/Progress.md). A green editor/game compile requires Visual Studio 2022 C++ tools and Git (both missing at first audit).

## Quick start (after tools are installed)

1. Install Git for Windows and Visual Studio 2022 with **Desktop development with C++** and Unreal’s required workloads.
2. Install a real Python 3.12+ (not the Microsoft Store stub) for contract validation.
3. Open `apps/game-unreal/FinalBell.uproject` in UE 5.8, allow module compile.
4. From repo root: `python -m pip install -e packages/domain-python` then `pytest tests/golden -q`.

FightVantage is a separate candidate source project; it is not renamed by this repository.
