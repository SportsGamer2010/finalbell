# Final Bell â€” Asset Checklist

Status key: Missing / Placeholder / In-editor / Verified

| Asset | Status | Owner | Notes |
| --- | --- | --- | --- |
| Ring map `Content/Maps/Ring_P1` | Optional | A | Runtime auto-arena on `/Engine/Maps/Entry` via `UFBP1ArenaBootstrap`. Optional save: `scripts/create_ring_map.py` |
| Placeholder fighter mesh/skel | In-editor | A | Epic SKM_Manny/Quinn_Simple + ABP_Unarmed loaded from Content/Mannequins; primitive body hidden when mannequin loads |
| Head/body/guard volumes | Missing | A | Required before marking contact Verified |
| Glove sockets + punch anims | Placeholder | A | Gloves attach to hand_l/hand_r; punch/guard still procedural offsets (not full punch anims yet) |
| Enhanced Input IMC + actions | Runtime C++ | A | `UFBRuntimeInputLibrary` builds IMC in memory (WASD/mouse + gamepad) |
| Broadcast camera BP defaults | Auto-spawned | A | Spawned by P1 arena bootstrap |
| Facial morph targets (L/R brow/eye/cheek, nose, mouth) | Missing | A | P2 spike FB-009 |
| Face UV bruise/cut masks | Missing | A | P2 |
| Niagara blood/sweat (bounded) | Missing | A | Cosmetic only; competitive damage unchanged |
| Training bag / mitt props | Missing | A | P2 |
| AI style animation sets | Missing | A | P2 |
| Fighter portraits (fictional) | Missing | B data / A UI | P3 records UI |
| Git LFS verified for binaries | Missing | Both | Check tracking before first asset commit |
| Fab free boxing anims/gloves/bag | Missing | A | Shortlist in docs/FabBoxingAssets.md — add in Editor only after Free confirm |
| Training heavy bag mesh | Placeholder | A | Procedural cylinder sway (`AFBHeavyBag`) until Fab bag assigned |

Never invent `.uasset` files in git. Document editor actions here when completed.


