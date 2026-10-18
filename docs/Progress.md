# Final Bell — Progress Log

## 2026-10-17 — P0 audit + first P1 source increment

### Audit evidence

| Item | Result |
| --- | --- |
| Existing Unreal game repo | **None** — only Cursor package under Downloads |
| Engine | **UE 5.8** at `C:\Program Files\Epic Games\UE_5.8` (UnrealEditor + UBT present) |
| Visual Studio / MSVC | **Installed** — VS 2022 BuildTools + Win10 SDK + .NET Framework 4.8.1 Dev Pack |
| Git | **Installed** (`Git.Git` 2.55 via winget); `git init` + LFS hooks OK |
| Python | **Store stub only** — contract pytest blocked until real Python 3.11+ |
| FightVantage source | **Not accessible** in this workspace |
| Binary assets | **None authored** — no fake `.uasset`/`.umap` created |

### Completed this increment

1. Monorepo at `C:\Users\Shadow\Projects\final-bell` with agreed layout.
2. Copied master prompt, design review, design reference, FeatureStatus, Cursor rule.
3. Created UE 5.8 C++ project `apps/game-unreal/FinalBell.uproject` + Game/Editor/Server targets.
4. Implemented smallest authoritative combat path: jab/cross/guard intents, stamina stub, reach/guard resolution, bout phases, broadcast camera, ring bounds, private-room subsystem.
5. Added v1 contracts for FighterTrueState, FighterReadiness, CombatIntent, FightResult + golden tests (unrun until Python installed).
6. Wrote Architecture, Progress, AssetChecklist, ContractRegistry, DecisionLog, Backlog, FeatureStatus updates.

### Build / test evidence

| Check | Evidence |
| --- | --- |
| UE module compile | **Succeeded** — `Build.bat FinalBellEditor Win64 Development` → `Result: Succeeded` (27s). Linked `UnrealEditor-FinalBell.dll`. Close Live Coding/editor before CLI rebuilds. |
| Python golden tests | **Not run** — real Python missing. |
| Two-client net smoke | **Not run** — needs `Ring_P1` map + Input assets. |
| Editor map/input assets | **Manual remaining** — see AssetChecklist. |

### 2026-10-17 — P1 auto-arena + runtime input

- `UFBP1ArenaBootstrap` spawns floor, light, ring bounds, opponent, broadcast camera on `/Engine/Maps/Entry`.
- `UFBRuntimeInputLibrary` builds Enhanced Input IMC/actions in memory (no input `.uasset` required).
- `scripts/dev_build_and_open.ps1` builds and launches the editor.
- Optional editor Python: `scripts/create_ring_map.py` to persist `/Game/Maps/Ring_P1`.

### Next two bounded tasks (file ownership)

| ID | Owner | Reviewer | Files | Acceptance |
| --- | --- | --- | --- | --- |
| FB-005b | A (Brian) | B | Jab/cross/guard timing + glove/block feedback on ring | Playtest: readable hit/block/miss; no double damage |
| FB-006 | A | B | 2-client listen-server smoke (pads 1/2) | Both windows same hit outcomes; notes in Progress |

Roadmap canvas: Cursor canvases → `FinalBell-Roadmap.canvas.tsx`. Logical next = **jab/block feel**, then **two-client authority proof**.

Shared contract: `CombatIntent` / `FightResult` schema `1.0.0` — do not bump without joint review.

### 2026-10-17 — Mannequin fighters (more realistic characters)

- Copied Epic TemplateResources Mannequins into `Content/Mannequins` (Manny/Quinn + Unarmed ABP).
- `AFBBoxerCharacter` loads `SKM_Manny_Simple` / `SKM_Quinn_Simple` + `ABP_Unarmed`, hides cylinder/sphere placeholders, attaches corner-tinted gloves to `hand_l`/`hand_r`.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (mannequin wiring compile + link).
- Playtest remaining: PIE visual check that both corners show skinned meshes walking with unarmed loco; gloves follow hands on jab/guard.

### Next two bounded tasks

| ID | Owner | Reviewer | Files | Acceptance |
| --- | --- | --- | --- | --- |
| FB-005b | A | B | Jab/cross/guard timing + glove/block feedback | Readable hit/block/miss on mannequin gloves |
| FB-006 | A | B | 2-client listen-server smoke | Both windows same hit outcomes |


### 2026-10-17 — Remote partner multiplayer path

- Docs: `docs/RemotePartnerMultiplayer.md` (Tailscale + listen-server); local PIE remains first in `docs/MultiplayerTest.md`.
- Scripts: `scripts/package_client.ps1`, `scripts/host_listen_server.ps1`, `scripts/join_listen_server.ps1`.
- Runtime: `FBHost` / `FBJoin` / `FBRoom` execs on `AFBSparringPlayerController`; `UFBPrivateRoomSubsystem` HostListenServer + JoinByAddress; `[URL] Port=7777`.
- Not done in this increment: actual partner session / cook archive verification (run `package_client.ps1` when ready to share a zip).


### 2026-10-17 — Modes, Undisputed controls, 3rd-fighter fix, training bag

- **3rd fighter:** GameMode now destroys all `FB_P1Opponent` / non-human boxers when Multiplayer or `>=2` humans/players; Tick safety purge.
- **Mode menu:** Slate overlay on host — 1v1 AI / 1v1 Multiplayer / Training; console `FBMode 1|2|3`.
- **Controls:** Undisputed Competitor mapping (jab/cross/hooks, R2 block, L2 weave, RS flick); see `docs/Controls_UndisputedCompetitor.md`.
- **Training:** `AFBHeavyBag` procedural hanging bag with spring sway; punches resolve against bag.
- **FAB:** research shortlist in `docs/FabBoxingAssets.md` (no invented `.uasset`).
- Build evidence: pending compile this increment.

- Build: `FinalBellEditor` Win64 Development **Succeeded** after modes/controls/bag increment.

### 2026-10-17 — Collision, close camera, facing, hook angle

- Body capsules block each other and the ring. Punches connect only when a punch volume overlaps the opponent.
- Fighters scale to 1.25 and the orbit camera sits closer (about 1.5–3.4 m) so gloves and faces read.
- Right stick yaws the shoulders inside a 50° cone. They always square back up to the opponent outside that cone.
- A hook that is weaved (L2/LT) or stepped under (~40 cm off the punch line) misses. The puncher keeps the facing they had when they threw it for 0.6 s. The defender's punches in that window deal 1.5× damage.
- Build: `FinalBellEditor` Win64 Development **Succeeded** after collision, camera, facing, and hook-angle increment.

### 2026-10-17 — Undisputed camera, heavyweight scale, ring light

Matched the Wilder/Holmes framing in `https://youtu.be/Hx1lo0UHjJY` (chest-high medium shot, fighter's back on the right, opponent facing the lens, waist crop).

- `AFBBroadcastCamera`: 58° FOV, 1.42 m behind the lead side, look at the opponent's high chest. No more midpoint zoom-out.
- Body scale 1.10 (~6'6" on the 180 cm mannequin). Opening gap 1.9 m. Gloves 30 cm.
- Ring is a 20 ft canvas with four ice/white ropes, a dark bowl, and warm overhead spots.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (50.7 s). PIE not re-checked this increment.

### 2026-10-17 — Brighter ring and a boxer with limbs

- Arena key light, shadowless fill, brighter walls, and a lifted exposure so fighters are not a mass of shadow.
- Placeholder body is now head, torso, two arms, two legs, gloves, and shoes in a guard. The unarmed mannequin stays hidden because it had no boxing pose.
- Jab, straight, lead/rear hook, and lead/rear uppercut each move the matching arm. Hold C (or LB) for the body line. Hold V (or RB) for a power shot.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (10.7 s). PIE not re-checked this increment.

### 2026-10-17 — Two fighters, all six punches on the number row

- `Ring_P1` has a placed boxer besides the spawned player and the AI. `EnforceRoster` deletes every boxer that is not a player, then solo sparring keeps exactly one opponent.
- Keys 1–6 throw jab, straight, lead hook, rear hook, lead uppercut, rear uppercut. The punch name is printed on screen. Each punch action has an explicit press trigger.
- Function map: `docs/CodeGuide.md`.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (25.5 s). PIE not re-checked this increment.

### 2026-10-17 — Skinned mannequin, guard, and six punch poses

- Blue corner uses `SKM_Manny_Simple`. Red corner uses `SKM_Quinn_Simple`. Both are driven by a poseable mesh, not the unarmed walk blueprint.
- Guard, high guard, weave, jab, straight, lead hook, rear hook, lead uppercut, and rear uppercut are bone poses blended from the punch phase. Gloves sit on the hand bones. The cylinder body remains only if those meshes fail to load.
- No boxing AnimSequence was authored. The pose is computed in `ApplySkinnedPose`.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (27.67 s). PIE not re-checked this increment.

