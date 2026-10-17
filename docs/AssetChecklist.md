# Final Bell — Asset Checklist

Status key: Missing / Placeholder / In-editor / Verified

| Asset | Status | Owner | Notes |
| --- | --- | --- | --- |
| Ring map `Content/Maps/Ring_P1` | Optional | A | Runtime auto-arena on `/Engine/Maps/Entry` via `UFBP1ArenaBootstrap`. Optional save: `scripts/create_ring_map.py` |
| Placeholder fighter mesh/skel | Placeholder | A | Default Character capsule/mesh until mannequin assigned |
| Head/body/guard volumes | Missing | A | Required before marking contact Verified |
| Glove sockets + punch anims | Missing | A | Final contact must not use full capsule |
| Enhanced Input IMC + actions | Runtime C++ | A | `UFBRuntimeInputLibrary` builds IMC in memory (WASD/mouse + gamepad) |
| Broadcast camera BP defaults | Auto-spawned | A | Spawned by P1 arena bootstrap |
| Facial morph targets (L/R brow/eye/cheek, nose, mouth) | Missing | A | P2 spike FB-009 |
| Face UV bruise/cut masks | Missing | A | P2 |
| Niagara blood/sweat (bounded) | Missing | A | Cosmetic only; competitive damage unchanged |
| Training bag / mitt props | Missing | A | P2 |
| AI style animation sets | Missing | A | P2 |
| Fighter portraits (fictional) | Missing | B data / A UI | P3 records UI |
| Git LFS verified for binaries | Missing | Both | Check tracking before first asset commit |

Never invent `.uasset` files in git. Document editor actions here when completed.
