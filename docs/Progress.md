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

### 2026-10-17 — Mannequin skeleton crash

- PIE asserted `SKM_Manny_Simple missing skeleton`. The mesh still references `/Game/Characters/Mannequins/Meshes/SK_Mannequin`, and the asset is at `/Game/Mannequins/Meshes/SK_Mannequin`.
- `TryActivateSkinnedMesh` assigns that skeleton before posing, and keeps the shape body if it is still missing. A package redirect maps the old mannequin folder onto the copied one.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (22.17 s). PIE not re-checked after this fix.

### 2026-10-17 — Ringside wide camera and cheek guard

- The broadcast camera sits outside the ropes, just above them, and looks slightly down so both fighters' feet and most of the 20 ft ring stay in frame. It slides with the action and eases to a profile, the way a ringside hard camera does.
- Guard hands sit at cheek height. The clavicles no longer shrug the gloves above the head. Uppercuts stop at the chin.
- Build: `FinalBellEditor` Win64 Development **Succeeded** (21.33 s). PIE not re-checked this increment.

### 2026-10-17 — Hooks land in front

- Hook hit spheres sat outside the lead shoulder, so they missed while jab and straight connected. They now finish in front of the opponent.
- The hook pose wings the elbow out so the bend reads from the ringside camera. Middle mouse throws the lead hook, and the mouse thumb button throws the rear hook.
- Build blocked: the editor had Live Coding active. Not recompiled in this step.

### 2026-10-18 — Local AI toolchain

- Added `Tools/Setup/Install-FinalBell-Toolchain.ps1`, `Tools/AI` orchestrator, Blender scripts, Cursor rules, and `docs/AI_Automation`.
- Installed GitHub CLI 2.102.0, Python 3.12.10, FFmpeg 9.0.2, and Node.js 22.23.2 with winget. Blender 4.5.10 LTS came from the official zip because the winget installer requested administrator approval.
- Git, Git LFS, UE 5.8.3, and VS 2022 Build Tools were already present and were not reinstalled.
- Evidence: 8 toolchain unit tests passed; 6 golden contract tests passed; Blender exported a scale-proxy FBX and the mesh check ran; orchestrator `check` passed. No `Build.bat` and no playtest in this step.
- Blockers: `gh` is not logged in. Figma, Linear, Adobe, DeepMotion, and Rokoko accounts are not connected. MetaHuman stays disabled in the project.
- Next: `gh auth login` when a pull request is needed, then keep combat changes in the existing boxer, camera, and ring classes.

### 2026-10-18 — Title screen and main menu

- Added a C++ UMG front end: title screen, main menu, settings, credits, and quit confirmation. No new widget assets.
- Packaged launches open `/Game/Maps/Ring_P1` with `FBMenuGameMode`. Editor PIE still starts in sparring. `-ShowMenu` opens the front end in the editor. `-SkipMenu` keeps a packaged build on the ring.
- Training travels to the map in Final Bell Menu settings (default `/Game/Maps/Ring_P1`) under `FBSparringGameMode`. Continue Career and New Career stay visible and unavailable. There is no career save.
- Settings cover master volume, window mode, resolution, graphics quality, and reduced motion. Display changes wait 15 seconds, then revert unless kept. Volume and reduced motion save to the `FBPlayerSettings` slot.
- Build: `Build.bat FinalBellEditor Win64 Development` **Succeeded** (21.14 s) and linked `UnrealEditor-FinalBell.dll`.
- Not run: PIE, keyboard/mouse/controller playthrough, resolution layout, settings persistence across a restart, gameplay return, and a packaged build.
- Next: play in editor with `-ShowMenu` and confirm the title handoff, Training load, settings apply/revert, and Escape back to the menu.

### 2026-10-18 — Body punches

- Holding L1, LB, or C now keeps the punch you threw and aims it at the ribs. Hooks are no longer rewritten into uppercuts.
- The torso bends with the punch, and the hit sphere drops to the body line. A high guard still does not block that shot. Uppercuts remain the right-stick down flick and keys 5 and 6.
- Build blocked: the editor had Live Coding active, so `Build.bat` did not run. PIE was not checked.

### 2026-10-18 — Upper body movement

- The guard pose now breathes, rolls the shoulders, counters with the head, and shifts the chest with each step. Punches still take over the torso while they are out.
- This is bone motion only. The capsule and punch volumes stay where they were.
- Build blocked again by Live Coding. PIE was not checked.

### 2026-10-18 — Head/body damage and waist coil

- The waist was stiff because punch yaw sat entirely on `spine_03` and the pelvis rotated against the punch. `BuildWaistTwist` now shares the turn across the hips, `spine_01`, `spine_02`, and `spine_03`. The stick still turns the capsule so punch spheres stay with the shoulders. The hips lag that turn and the chest catches back up.
- Server damage is three pools: head, body, and stun. Body damage slows energy refill and outgoing power. Stun slows the step and outgoing power, then fades. Filling a pool does not end the bout. The flinch is bone motion. The capsule size is unchanged.
- Reading notes: `docs/DamageAndWaist.md`. Decision D-012.
- Build: `Build.bat FinalBellEditor Win64 Development` **Succeeded** (56.24 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The waist coil and the head/body numbers have not been watched in a fight.

### 2026-10-18 — Meters and the 1v1 gym

- Corner bars show stamina, head, body, and stun for you and the opponent after the mode menu closes. They read the replicated pools. Slate, using the broadcast colors. No Widget Blueprint.
- 1v1 vs AI replaces the dark bowl with a wood floor, brick walls, a low ceiling, beams, windows, heavy bags, a bench, a bucket, a mirror, and posters. The ring size stays. Multiplayer and training keep the dark bowl. Lights go warm only in the gym.
- Reading notes: `docs/DamageAndWaist.md` and `docs/CodeGuide.md`. Decision D-013.
- Build: `Build.bat FinalBellEditor Win64 Development` **Succeeded** (24.91 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The bars and the gym have not been watched in the editor.

### 2026-10-18 — 3:00 bout, knockdown count, three judges

- Spending burst energy to empty knocks that fighter down in a live bout. The camera cuts low, looking up at a referee built from basic shapes. Both sticks must sit on two server-owned rotating marks before the count of 10. WASD is the left circle. Arrow keys are the right circle. Beating the count restores enough energy to stand and fight. The mesh lies down. The capsule and punch spheres stay the same size.
- The bout is one 3:00 round. The clock pauses during the count. If it expires with nobody counted out, three judges (Harris, Cole, Reed) score a 10-point must card from punches, head damage, body damage, and knockdowns. A failed count is a knockout card and does not invent round scores. Training does not run the clock or the count. The first AI knockdown gets up on its own. The second does not.
- The clock, the circles, and the card are Slate, in the broadcast black, gold, cream, blue, and red. No Widget Blueprint.
- Reading notes: `docs/BoutAndScorecard.md` and `docs/CodeGuide.md`. Decision D-014.
- Build: `Build.bat FinalBellEditor Win64 Development` **Succeeded** (36.87 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The count, the stick circles, the clock, and the scorecard have not been watched in a fight.

### 2026-10-18 — Visible damage and Competitor precision

- Head damage grows a cheek bruise, a swollen eye, a bloody nose, and a brow cut. Body damage grows a rib bruise and darkens the torso. Those meshes have no collision. The capsule and the punch spheres stay the same size.
- A lot of body damage now cuts walk speed by up to 55% (floor about 34%) and stretches punch startup and recovery up to about 1.85 times.
- Punches require the Competitor button and a 26-degree stick cone: Square northwest jab, Triangle northeast straight, X west lead hook, Circle east rear hook, X+Square southwest lead uppercut, Circle+Triangle southeast rear uppercut. The stick alone, aimed down, is a push.
- L3+R3 starts a clinch when close. Punches still work inside it, shorter and lighter. The left stick fills a turn meter; at 100 the opponent is spun. The referee breaks a clinch at about 4.6 seconds. A third clinch, or a second headbutt or low blow, deducts a point on the card.
- Also mapped: R1+move diagonal dodge, L3+move slip, D-pad loose movement, stance, taunt, headbutt, and low blow. Keyboard letters are in `docs/Controls_UndisputedCompetitor.md`.
- Reading notes: `docs/Controls_UndisputedCompetitor.md` and `docs/CodeGuide.md`. Decision D-015.
- Build: `Build.bat FinalBellEditor Win64 Development` **Succeeded** (28.05 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The bruises, the stick cones, the clinch, and the point deduction have not been watched in a fight.

### 2026-10-18 — CompetitiveFair builds

- The balance document is the only model. Nineteen ratings, 50–95, exactly 600 points to fight. The earlier 1100-point check is gone.
- Eight named division fighters, four 600-point plans, a create screen, and a save of up to 10. 1v1 AI and `FBMode 1` open that screen. The server refuses a build that is not exactly 600 or that crosses weight classes.
- Hand power, chin, body, guard, stamina, footwork, accuracy, courage, cuts, and bruises layer onto the existing boxer. The capsule and punch spheres are unchanged. Complexion, hair, face, tattoos, and muscle are presentation.
- The card adds one line from the bout's own punches, misses, blocks, body shots, guard breaks, and knockdowns. A created fighter stores Career XP, the result, and 20 retrain points. The ratings are not raised by the win.
- Between-round stamina recovery is implemented. This bout is still one 3:00 round, so that rest does not run.
- Reading notes: `docs/FighterBalance.md` and `docs/CodeGuide.md`. Decision D-016.
- `python Tools/AI/scripts/check_fighter_balance.py` printed `CompetitiveFair: 19 ratings, budget 600, 4 presets, 8 division fighters, one cost curve.` Exit 0.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Succeeded** (12.87 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The fighter screen, the stat differences, and the card line have not been watched in a fight.

### 2026-10-18 — Punch throw, create-screen controls, MetaHuman assets not built

- A face button or keyboard punch was refused unless the right stick was already inside the accuracy cone. The source now throws the matching punch anyway, and keeps the cone as a 6% bonus. That change is not in the running editor yet.
- Create a Fighter source now saves the identity, body, face, skin, hair, and rating pages described in `docs/CodeGuide.md`. The mannequin preview uses tint, hair, beard, tattoo, scar, and glove proxies. The capsule is unchanged.
- `FinalBell.uproject` had been saved as UTF-16. It was rewritten as UTF-8 with MetaHuman, MetaHumanCharacter, MetaHumanCrowd, and MetaHumanCalibrationDiagnostics still enabled.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Failed** (`Result: Failed (OtherCompilationError)`). Unreal said Live Coding is active. Two `UnrealEditor.exe` processes have this project open. Python remote execution found no editor node, so `Tools/Unreal/create_metahuman_roster.py` did not run. No MetaHuman `.uasset` was created.
- PIE was not run.

### 2026-10-18 — Heavy Bag Drill

- Front-end menu and the sparring mode menu both list **Heavy Bag Drill**. Console `FBMode 3` is the same choice. It starts `EFBPlayMode::Training` against `AFBHeavyBag`. It does not start a bout, the 3:00 clock, or knockdowns.
- The bag is a 40 kg physics body. A locked linear constraint hangs it from a fixed anchor. Swing is limited to 50 degrees and twist to 35. An angular drive returns it to straight. A punch calls `ApplyStrike`, which adds impulse at the contact point. The bag ignores the pawn channel. The capsule and punch spheres are unchanged.
- MetaHuman Animator is the plugin named `MetaHuman` (`MetaHuman.uplugin`, friendly name MetaHuman Animator). It was already enabled. `UMetaHumanPerformance` takes `UFootageCaptureData` for monocular footage, or a sound wave for audio. It has no YouTube URL field. Body tracking is a checkbox on monocular footage (`bBodyTracking`, modes None / Realtime / Offline). Solving and export run in the editor on a Performance asset after Capture Manager has ingested a local take. No local video was supplied, so no animation asset was created.
- Page fetches: `O5DaX4Y_EQQ` title is "JARON BOOTS ENNIS STAYING SHARP AND IN SHAPE". The still shows a boxer in a ring holding a small pad, not a hanging bag. `uWvOnR0-gd4` still shows hanging cylindrical bags in a gym and the on-screen line "11. Timing the bag swing back on jab then 2 L step out". The spoken page text includes "Two three", "Good combination", and "3 4 5". The videos themselves were not played, so swing, chain links, contact, and rebound were not watched.
- First build failed on two earlier source errors (`FFBFighterRecord` has no `Nickname`; the fighter screen used `AFBBoxerCharacter` without its header). Those were corrected. Second build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (13.08 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The hanging bag has not been watched in the editor.

### 2026-10-19 — Arena lights stay movable

- The only lighting message in `Saved/Logs/FinalBell.log` (and the 2026-10-19 backups) is `PIE: Warning: Mobility of /Game/Maps/UEDPIE_0_Ring_P1.Ring_P1:PersistentLevel.DirectionalLight_0 : LightComponent0 has to be 'Movable' if you'd like to move.` Map check was 0 errors, 0 warnings. There was no Lightmass, Lumen, overlap, or "LIGHTING NEEDS TO BE REBUILT" line.
- Cause: `SpawnLighting` calls `SetActorRotation` on the map directional light. Engine defaults for directional, spot, and point lights are Stationary, and `r.AllowStaticLighting` is already false. The rotation is refused until the component is Movable. A shadow-casting stationary light with no built channel is also the case that draws the light-error icon.
- `UFBP1ArenaBootstrap::SpawnLighting` now sets Movable on the key, fill, spots, center point, and gym bulbs before that rotation. The heavy-bag drill adds no lights. Hitboxes, bout rules, and punch input were not changed.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Failed** (`Result: Failed (OtherCompilationError)`). Unreal said Live Coding is active. `UnrealEditor` (pid 6480) has the project open. The editor was not closed.
- PIE was not run. The warning will stay until this binary is compiled and PIE is started again.

### 2026-10-19 — Smoother steps and upper-body defense

- Watched the Overhand Interactive short `_Xcu40JWxRk` (about 21 seconds, on-screen line "NOTE: THIS IS PRE-ALPHA GAMEPLAY"). Frames show a bent-knee guard, steps in and out, hip and shoulder rotation on punches, deep forward folds with the head down (about 2.0s, 4.4s, 9.2s, 13.8s, 16.0s), a lean of the chest away from a punch (about 6.0s and 12.2s), and the head leaving the punch line (about 8.4s, 11.4s, 16.8s). That is not a claim that Final Bell already moved this way.
- The capsule still owns the step. Acceleration, braking, and the stick ease in and out. Hips lag a yaw change through `BuildWaistTwist`. Feet stay on the same IK targets.
- Slip left and slip right are L3 / Left Alt with the stick to that side. A weave, or a slip pushed forward, rolls under (pelvis through chest, lead or stick-side shoulder drop). A slip or weave pulled back leans away. `neck_01` counters the bend. `AuthoritySlip` still steps the capsule and still grants the angle. Decision D-018.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (55.65 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The slip, roll, lean, and the eased step have not been watched in the editor.

### 2026-10-19 — Poster main menu

- The front-end menu and the sparring mode menu now share one poster: arena photo, **FINAL** / **BELL** on the upper left, red bar on the selected row.
- Photo: `apps/game-unreal/Content/UI/FB_MenuArena.jpg`. The left side of that file is flat black so "FINAL ROUND", "CAREER MODE", "TRAINING", "SETTINGS", and "EXIT GAME" are not in the texture. `UFBMenuWidget` draws the packaged / `-ShowMenu` screen. `AFBSparringPlayerController::ShowModeMenu` draws the same poster in PIE.
- Front-end rows: **1V1 VS AI** (opens fighter select), **1V1 MULTIPLAYER**, **HEAVY BAG DRILL** (`EFBPlayMode::Training`, no 3:00 clock), **SETTINGS**, **CREDITS**, **QUIT GAME**. The sparring poster lists the first three. Create a Fighter stays inside 1v1. Career placeholders are not on the poster. Editor PIE still skips the front end unless `-ShowMenu`.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (28.24 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run.

### 2026-10-19 — Fighter select and styles

- The strange screen was the debug slate in `Combat/FBFighterSelect.cpp`. It filled the view, said CHOOSE YOUR FIGHTER, used the engine's default buttons, prefixed every row with YOU or THEM, and put an OPP button on each row. It appeared the moment 1v1 was chosen. There was no fade, and it did not use the arena poster.
- 1v1 vs AI now takes that poster down and slides one fighter screen in over the same arena photo (0.36s, or immediately when reduced motion is on). The list is the eight premades, with name, division, style, and the ratings that define the style, plus Create a Fighter. Next Opponent stays inside the same class group. Fight and Back call `HideFighterSelect`, which sets `FInputModeGameOnly`. Multiplayer still applies that mode and does not open this screen. The ten-save cap and CompetitiveFair are unchanged.
- Each premade is its own 600-point build. Hale is the slugger, Petrov the in-fighter, Voss the counterpuncher, Quinn the boxer-puncher, Marte the swarmer, Kane the out-boxer, Abebe the high guard, and Solano the southpaw. Voss is a southpaw whose identity is the counter. Marte is not a second copy of Petrov. The four creator presets remain shortcuts, not the roster. Capsule, punch spheres, and BodyScale were not changed.
- `python Tools/AI/scripts/check_fighter_balance.py` from the repo root exited 0.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (12.13 s) and linked `UnrealEditor-FinalBell.dll`. The balance, fighter-select, and sparring-controller files were compiled in that target.
- PIE was not run.

### 2026-10-19 — Glove window and defense timing

- The glove was already `DoesPunchConnect`, a server sphere on the capsule. It now stays live for the whole active window. A whiff is recorded when that window ends.
- A high guard or a weave counts only if it was already held when the window opened (`IsHeldDefenseOnTime`). A slip, roll, or lean beats a head shot only while `IsHeadEvasionOnTime` is true. Those poses do not move the capsule or the punch spheres. Body shots are not slipped by the pose. A held guard does not expire.
- No `UPhysicalAnimationComponent`. `ApplySkinnedPose` still writes the bones every frame. Decision D-019. GAS was already enabled in the project and was not wired to the fighter card.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (39.25 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run.

### 2026-10-19 — Fight start, pause, and pad menus

- Fight did nothing useful because the bout phase was already past pre-bout before the button ran. The arena timer (and a saved Versus AI mode) called `StartBout` while the fighter screen was up. `StartBout` then returned immediately, and the 1v1 path destroyed the ringside camera as the screen closed.
- Fight now stays on that screen until `ServerStartCompetitiveBout` accepts the pair. Acceptance calls `ApplyPlayMode(VersusAI)`, which resets knockdown, clinch, and the clock, places both boxers, and calls `StartBout`. A refused build stays on the screen with the reason. Input goes to `FInputModeGameOnly` so punches work. The stick cone is unchanged.
- Options or Escape during a fight or the heavy bag opens pause: Resume, Main Menu, Change Mode, Quit. The host pause freezes the local bout and does not write a win or change damage. A remote client cannot freeze the host. Main Menu clears the bout and returns to the front-end poster. Change Mode clears it and shows the three modes without exiting. Quit confirms, then uses the front-end exit.
- D-pad, left stick, arrows, and WASD move the red bar on the front end, the fighter screen, and pause. Cross, Enter, or Space accepts. Circle or Escape backs out. Options pauses and resumes. With the menu closed, Cross, Circle, the shoulders, and the right stick stay boxing inputs.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (13.06 s) and linked `UnrealEditor-FinalBell.dll`. The first pass failed on an uncaptured `FocusIndex` in the fighter list; that capture was added and this second pass succeeded.
- PIE was not run.

### 2026-10-19 — Four modes and the MetaHuman startup lines

- Front end, the sparring poster, and pause Change Mode now list **1V1 SPARRING (AI)**, **1V1 MULTIPLAYER**, **HEAVY BAG TRAINING**, and **1V1 CAREER**. Settings, Credits, and Quit stay on the front end only. Pause stays Resume, Main Menu, Change Mode, Quit.
- Sparring is the existing fighter-select path into `ServerStartCompetitiveBout` / `ApplyPlayMode(VersusAI)`. Multiplayer is unchanged. Heavy bag is still `EFBPlayMode::Training` against `AFBHeavyBag`, with no 3:00 clock and no knockdowns.
- Career is `EFBPlayMode::Career`: one 3:00 bout under the same server rules. Exactly one ready created fighter starts against the first ready premade in that class group. Zero or several created fighters open fighter select, and the opponent stays a premade in that class. `AwardCareerFromCard` writes wins, losses, draws, and +25 Career XP on that created fighter in the `FBFighterRoster` save, then 20 retrain points. Premades are skipped. Nothing is copied into an official ranked pool. FB-ENH items were not marked verified.
- MetaHuman startup, `Saved/Logs/FinalBell.log` at 2026-10-19 15:17. The log is a full startup, so the older backup was not the source. The project is not requesting a path it owns. Enabled plugins include MetaHuman, MetaHuman Character, MetaHuman Crowd, MetaHuman Calibration Diagnostics, MetaHuman Generator, MetaHuman Runtime, and MetaHuman Live Link. MetaHuman Animator is not its own `.uproject` entry; its modules load from `Engine/Plugins/MetaHuman/MetaHumanAnimator` because MetaHuman depends on them. DNACalib and MetaHuman Identity loaded. There is no `MetaHumanData` error. There is no `MH_*.uasset` in the project. The lines are:
  - `[2026.10.19-15.17.44:565][  0]LogMetaHumanCharacterEditor: Warning: MetaHuman Optional Content folder not found. MetaHuman Creator plugin initialized with limited features.`
  - `[2026.10.19-15.17.45:279][  0]LogSlateStyle: Warning: Unable to find Slate Widget Style 'MetaHumanAuthenticationMenu'. Using FButtonStyle defaults instead.` (twice)
  - `[2026.10.19-15.17.45:279][  0]LogSlate: Could not load file for Slate resource: [../../../Engine/Plugins/MetaHuman/MetaHumanSDK/Content/Intro_1.png]`
  - `[2026.10.19-15.17.45:280][  0]LogSlate: Could not load file for Slate resource: [../../../Engine/Plugins/MetaHuman/MetaHumanSDK/Content/Intro_2.png]`
  - The same two Intro png lines repeat at 15:17:45.560 and 15:17:50.888.
  - `[2026.10.19-15.18.00:383][  0]LogMetaHumanCharacterEditor: Warning: MetaHuman Optional Content folder not found. MetaHuman Creator plugin initialized with limited features.`
  - `[2026.10.19-15.18.00:383][  0]LogPython: Warning: [MetaHumanGenerator] MetaHuman optional content not available, toolset not registered`
- Cause: the Creator plugin and the MetaHuman Generator startup script look for Epic's optional MetaHuman content pack and do not find it. The intro images and the authentication menu style are engine plugin content under MetaHumanSDK, not a Final Bell asset. Fix: in the Epic Games Launcher, install the MetaHuman content for UE 5.8, or open MetaHuman Creator in this editor and download the optional content when it asks. Do not disable the plugins. `create_metahuman_roster.py` has not made a character; that is not what these lines are reporting.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Failed** (`Result: Failed (OtherCompilationError)`, 28.05 s). UHT wrote the generated files, then Unreal said Live Coding is active. The editor was not closed.
- PIE was not run. The new mode labels will stay on the previous binary until this compile finishes. Press Ctrl+Alt+F11 in the editor to compile, or close the editor and run Build.bat again.

### 2026-10-19 — Two premades per division, and the opponent enters the ring

- "The opponent did not enter the ring" came from `ApplyPendingBuilds` when no non-player boxer was left. Editor PIE is a two-client listen server. `PlaceFightersForMatch` skipped the AI spawn when `GetNumPlayers()` was 2, and `EnforceRoster` then deleted every non-player boxer. The selected premade never got a body. The next tick also flipped sparring to multiplayer and deleted an AI if one had been spawned. Closing the fighter screen was not what destroyed the camera; the roster purge removed the opponent before `StartBout`.
- Sparring and career now always spawn one `FB_P1Opponent`, possess it with an AI controller, and keep that actor even when a second PIE client is connected. The confirming controller's pawn gets the player build. `StartBout` still runs only after that pair is accepted, from pre-bout. An illegal pair stays on the fighter screen with the budget or class reason. Hiding the screen still sets `FInputModeGameOnly` and does not destroy the broadcast camera or the opponent.
- Career no longer starts against the first premade in the class. Sparring and career both open fighter select. The opponent list and Next Opponent move only through ready fighters in the same class group. Career stays on premades.
- Each division now has two 600-point premades. The new ones are Caleb Drummond (heavyweight brawler), Idris Crowe (cruiserweight jab technician), Mateo Ruiz (light heavyweight power puncher), Andre Pell (middleweight body puncher), Chris Adel (welterweight slick boxer), Luis Ortega (lightweight inside fighter), Kenji Sato (featherweight speedster), and Pavel Nemec (bantamweight peek-a-boo). The original eight are unchanged. Premades still do not count toward the ten-save cap. No MetaHuman asset was added. The capsule and punch spheres were not changed. FB-ENH items were not marked verified.
- `python Tools/AI/scripts/check_fighter_balance.py` from the repo root exited 0. It printed `CompetitiveFair: 19 ratings, budget 600, 4 creator presets, 16 styled division fighters, two in each division, one cost curve.`
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (53.90 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run.

### 2026-10-19 — 1v1 vs AI had a third boxer and a dark window

- Cause of the third person, from `apps/game-unreal/Saved/Logs/FinalBell.log` at the Fight press: `[2026.10.19-16.26.05:841][809]LogTemp: Warning: FinalBell PlaceFighters mode=1 humans=2 players=2` and, same timestamp, `FinalBell P1 arena ready (player=FBBoxerCharacter_1 opponent=FBBoxerCharacter_3 spawnOpp=1)` then `Final Bell: 3:00 bout started. mode=1.` Mode 1 is Versus AI. The roster purge only removed `FBBoxerCharacter_0` (`FinalBell: removing extra fighter FBBoxerCharacter_0`). Both PIE pawns stayed because they are player-controlled, and `FB_P1Opponent` was spawned on top. That is three boxers. The same log has `[2026.10.19-16.24.52:487][  0]LogConfig: Set CVar [[r.AllowStaticLighting:0]]` and no `SpawnLighting` line, because lighting was not logged. The game mode runs only on the listen-server window, so the second PIE window never made `DirectionalLight_0` movable and never built the gym. The fighter screen is a full-screen slate at alpha 1, and it was added and removed through `GEngine->GameViewport`, which is not a stable window when two PIE clients exist.
- 1v1 sparring and career now keep the controller that confirmed the pair and bench every other player pawn: unpossess, destroy that pawn, spectate, and hide the spectator pawn. The AI opponent still spawns. Multiplayer does not bench and still does not spawn the AI. Restart of a benched controller does not give them a new boxer.
- When the bout is accepted, each local window hides the fighter slate through that window's own viewport, sets `FInputModeGameOnly`, builds the old-school gym, forces key, fill, spots, and bulbs to Movable, and sets the view to a ringside broadcast camera. The camera is not owned by the pawn that gets benched, so hiding the screen does not destroy it.
- First `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Failed** (`Result: Failed (OtherCompilationError)`, 64.17 s). UHT had named the four combat struct bodies two lines above `GENERATED_BODY()`. Those braces were moved onto the struct line. Second build **Result: Succeeded** (45.43 s) and linked `UnrealEditor-FinalBell.dll`. The editor was not running. PIE was not run.

### 2026-10-19 — Undisputed PS5 Competitor controls and a readable roll

- L1, R1, L2, R2, and the face buttons were on a Pressed trigger, so a hold ended on the next frame. Block, weave, body, and power never stayed down. They now use a Down trigger for the whole press. A tap of L1 or R1 under a quarter second, with no punch, rolls left or right. Holding L1 still aims the glove at the ribs. Holding R1 is still the power shot. R2 with the stick down is the body block.
- A side roll commands 48° through the trunk (was 32°). The neck gives back 0.40 (was 0.70), so the head stays about 39° off the punch line. The low shoulder drops about 17 cm. Lean back is -58° (was -34°). A roll under is 74° forward and 42° to the side (was 50° and 24°). The capsule and the punch spheres do not follow the bones.
- Reading notes: `docs/Controls_UndisputedCompetitor.md` and `docs/CodeGuide.md`. Decision D-023.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (38.58 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The rolls and the shoulder buttons have not been watched in a fight.

### 2026-10-19 — L2 and R2 are the analog triggers

- The fight map had weave on `Gamepad_LeftTrigger` and block on `Gamepad_RightTrigger`. A DualSense does not send those button events. `DefaultInput.ini` sends L2 as `Gamepad_LeftTriggerAxis` and R2 as `Gamepad_RightTriggerAxis`. Those axes are now bound, with a 0.35 deadzone. L1 and R1 are unchanged.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (9.15 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run.

### 2026-10-19 — Style AI and the three scoring pillars

- `AFBBoxerAIController` chooses the opponent's next step and the next `FFBCombatIntent`. The default AI controller did not move or punch. The goal is to win the bout: land clean, avoid damage, keep enough energy to stay standing, and do not punch a fighter who is down. Training and the heavy bag do not run it.
- The style name picks the range, the punch mix, and whether they lead. Feet, power, guard, accuracy, and stamina stay on the existing 600-point ratings. There is no second budget. A clean punch is still 1 punch credit and a block is still 0.25.
- Harris, Cole, and Reed still write one 10-point must card, and only when both fighters are up at the bell. Harris weights punch count. Cole weights ring generalship. Reed weights knockdowns. A knockdown that is beaten stays on that card. A count of ten is still the knockout card.
- Reading notes: `docs/CodeGuide.md` and `docs/BoutAndScorecard.md`. Decision D-025.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded**. The first pass failed because `FindOpponent` is protected. The follow-up linked `UnrealEditor-FinalBell.dll`. FB-ENH items were not marked verified.
- PIE was not run.

### 2026-10-19 — Split fighter select

- 1v1 Sparring (AI) and 1v1 Career show two panels on the same screen. The left is the player's fighter. The right is the opponent. Each side lists all 19 CompetitiveFair ratings from the fighter record and a short Keys to winning block aimed at the other style. The lines live in `Combat/FBFighterSelect.cpp`. The opponent list is still ready fighters in the same class group. Fight still calls `ConfirmCompetitiveBout`.
- Reading notes: `docs/CodeGuide.md`. Decision D-024.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** and linked `UnrealEditor-FinalBell.dll`. `FBFighterSelect.cpp` compiled clean. PIE was not run. FB-ENH items were not marked verified.

### 2026-10-19 — Heavy bag room

- The training bag looked like a line because the thing you could see was the chain. Five cubes were scaled to `(0.045, 0.045, 0.28)` on the 100 cm engine cube, so each link was 4.5 cm across, and they ran about 95 cm from a hang point above the bag down to its top. The leather scale `(0.85, 0.85, 1.7)` was set only in the constructor. That actor was spawned at `(220, 0, 160)`, inside the 20 ft ring, and training called `EnsureArena` with the dark bowl. The ringside camera sits about 9 m back to frame the whole ring, so the bright thin chain was the shape in that shot.
- Heavy bag training now stands in a bag room north of the bout gym. The room center is `(0, 2800, 0)`. The old-school gym wall is at Y=1400, and the bag room's south wall is at Y=1900, so there is a gap between them. The bout ring, ropes, and `AFBRingBounds` stay at the origin for sparring and career. Training does not clamp the fighter back into that ring, and it still does not run the 3:00 clock or knockdowns. `/Game/Maps/Ring_P1` was not rebuilt.
- The active bag is a leather cylinder 46 cm across and 132 cm tall, with end caps and bands, on a 34 cm chain from a beam at Z=252. The bottom hangs about 86 cm off the floor, waist to thigh on the mannequin. Four neighbor bags use the same shape and do not simulate or block. Centers are 340 cm apart, so a 2 m circle around the active bag does not meet them. Lights in the room are Movable.
- A punch that overlaps the leather calls `ApplyStrike` on the server with the punch-sphere center. That adds an impulse at the contact point, so the 40 kg body swings and twists. An angular drive brings it back. The bag overlaps the pawn instead of blocking it, so it does not push the capsule or the punch spheres.
- Reading notes: `docs/CodeGuide.md`. Decision D-026.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Failed** (`Result: Failed (OtherCompilationError)`, 9.49 s). UHT wrote the generated files, then Unreal said Live Coding is active. The editor was not closed. PIE was not run. Press Ctrl+Alt+F11 in the editor to compile.

### 2026-10-19 — Input buffer, corners, swept contact, bout gaps

HEAD was `065fcd25ede8a86c6d0f3f269caa2913a7cc483e` on `cursor/p1-visible-arena-dualsense-input`. That commit already had the bag room, Down triggers for block/weave/body/power, analog L2/R2, shoulder-tap rolls, style AI, the three judge pillars, the benched extra PIE pawn, movable gym lights, the split fighter screen, 16 premades, and server-owned hits. Those were not redone. No client damage field was added. `scripts/create_ring_map.py` was not run. No `.uasset` was created. `/Game/Maps/Ring_P1` was already in Content.

- Stage 1 baseline: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (27.50 s) and linked `UnrealEditor-FinalBell.dll`. The editor was not running, so Live Coding did not block it.
- Stage 2: block, weave, body, and power already used `UInputTriggerDown`. A held guard or modifier is remembered through Startup, Active, and Recovery and applied when the phase is idle again. One attack can queue in the last 125 ms of recovery, then it expires. A development message says accepted, buffered, or rejected and why. It does not print a punch name as if a rejected punch started. The right stick is mapped once. Below 0.55 it only yaws the shoulders. At or above 0.55 it throws one punch and does not also yaw. The 0.78 punch-stick gate is gone. Keys 1–6 and the face buttons still throw.
- Stage 3: blue is the northwest corner and red is the southeast corner, inside `AFBRingBounds`, inset by the capsule radius, facing center. `SendFightersToCorners` is the first bell, the rematch, and the return after the round. Versus AI still places the confirming player and one AI. Multiplayer still places two humans and no AI. A later login does not rebuild the floor or the lights when that shell is already there.
- Stage 4: the glove sphere sweeps across the active window. Head, torso, and guard are query-only volumes. A high guard in the way blocks a head shot. A body shot ignores that high guard. Damage is once per attacker, attack, and defender. The movement capsule is still 34 x 90, and the punch radii are still 40 / 48 / 50. `fb.DebugContact 1` draws the sweep. The bout HUD does not.
- Stage 5: a new attack is refused during a knockdown, a rest, after the card, and while the bell is waiting. A punch already in Startup or Active still resolves, and the bell waits for that window. Pause on the card is Rematch, Main Menu, and Quit, on the existing pause path. The AI still sends `FFBCombatIntent`. Rounds were not added. FB-ENH-01 through FB-ENH-06 were not marked verified. Ranked play was not started.
- Stage 6: the gym north wall and the bag-room south wall are split, with a wood hall and one movable light between them. The ring stays at the origin and the bag row stays at Y=2800. This is still engine cubes. Still missing: brick and wood textures, glove and bell and crowd audio, a crowd, ropes that are not cubes, and any licensed gym prop. A generated picture was not used as the level.
- Stages 2–6 compiled together: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (46.89 s) and linked `UnrealEditor-FinalBell.dll`.
- PIE was not run. The buffer, the corners, the sweep, the rematch, and the hall have not been watched in a fight. Decision D-027.

### 2026-10-19 — Bag room balance

The heavy-bag shell read as off balance because the pieces did not share one floor or one subject.

- The gym floor and the wood hall both top out at Z=-90. The bag-room floor was centered at Z=-12 with thickness 24 cm, so its walk surface was Z=0 and its underside was Z=-24. That is a 90 cm step up from the hall, with 66 cm of air under the slab. The south wall bottom was Z=-5, so the brick floated 85 cm above the hall.
- The room was 24 m by 18 m (scale 24 by 18 on the 100 cm cube), center (0, 2800, 0). Five bags hung through the middle at Y=2800, 340 cm apart, not on a wall. The beam was 20 m long, so each end stopped 179 cm short of the side walls, and its top (Z=270) sat 54 cm under the ceiling. It was a floating bar.
- The mannequin capsule is `InitCapsuleSize(34, 90)`, then `BodyScale` 1.10, so the standing body is radius 37.4 cm and half-height 99 cm (198 cm tall). Training placed the feet at Z=4, 4 cm above that raised floor, and 115 cm south of the bag center. The gap from the capsule to the 46 cm bag was about 55 cm. Yaw 90 faced the bag. The pawn was not inside a bag and not in a corner.
- The training camera sat 340 cm off the fighter-bag line, and 340 cm is the old row spacing. The lens landed at about X=-340, on the west neighbor, looking along the row.
- The ring spots attenuate at 2200 cm and the hall bulb at 900 cm. Neither reaches Y=2800. The bag-room bulbs were Movable, but they lit the middle of the empty box.

The shell now uses the gym walk surface. Interior is X ±560 cm and Y=1900 to 2680, with walls and a ceiling that meet that floor. The doorway matches the 280 cm hall, and the hall has side walls, a ceiling, and floor strips so the corridor is closed during training. The beam runs into both side walls. Five bags hang on it at Y=2525, spacing 200 cm, hang Z=162, so the leather bottom stays 86 cm off the floor. The fighter stands at the bag's X, 175 cm south, feet on Z=-90, facing +Y. About 4.1 m of floor is behind them and about 2 m of air is between them and a neighbor bag. The camera is 300 cm to the side and 290 cm back from the pair, clamped inside the walls, and two Movable spots plus three bulbs light the row and the fighter. The bout ring was not moved. Training still has no 3:00 clock and no knockdowns. `ApplyStrike` is unchanged, and the bag still overlaps the pawn.

- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (25.69 s) and linked `UnrealEditor-FinalBell.dll`. Live Coding did not block it.
- PIE was not run. Decision D-028.

### 2026-10-19 — Ringside zoom follows the two fighters

The fight camera stays on `AFBBroadcastCamera`. It measures the center-to-center gap of the two live boxers (the human and the AI, or the two humans) and ignores a benched or destroyed PIE pawn. **110 cm** is the clinch. **760 cm** is the far end, the diagonal of the ±270 cm playable box. The lens moves along the existing ringside offset: about **5.1 m** from ring center when they are at 110 cm on a flat side at 16:9, and about **8.9 m** at the wide end (the previous 62° ringside distance). It stays at least 120 cm outside the 305 cm rope line, 170 cm off a fighter, and inside ±1300 cm so it does not enter the ring or sit in the gym wall. The gap eases at 2.8 (about 0.35 s), then the camera eases onto that spot. Fight FOV stays **62°**, may tighten to **58°** when the ropes block a closer dolly, or open to **68°** when a wall blocks a wider one. The knockdown cut stays **42°** and does not take this zoom. Heavy-bag training still frames the fighter and the active bag. Hits, stamina, corners, and the server were not changed.

- Reading notes: `docs/CodeGuide.md`. Decision D-029.
- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (27.10 s) and linked `UnrealEditor-FinalBell.dll`. The first pass failed to compile because `AutoFindFighters` was missing from the header. Live Coding did not block it.
- PIE was not run.

### 2026-10-19 — DualSense did nothing on the menu or in the fight

The pad reached neither place. Menus used `FInputModeUIOnly`, and that mode calls `SetIgnoreInput(true)` on the viewport, so the player controller never saw face buttons, the D-pad, or the sticks. The menu widget only received a key if it already had focus, and PIE kept moving focus onto the viewport. Solo PIE also rewrote the pad's device id to the keyboard, so the confirming local player was not the device that owned the event.

The fight mapping was only added, never removed. A menu could leave it off the local player that confirmed the bout, or leave it on after the window lost focus. The right-stick 0.55 punch gate was only on the right stick. It was not what silenced the left stick, the D-pad, or the face buttons. Those were dead because of the input mode and because the stick was gated three times: AxisConfig at 0.38/0.40, a second radial dead zone of 0.38, and `Move` zeroing anything under 0.35. A normal tilt never came through.

`DefaultInput.ini` had also assigned DualSense buttons in Xbox order. On this pad the HID order is Square, Cross, Circle, Triangle, L1, R1, then the digital L2/R2 clicks, then Create, Options, L3, R3. Options, L3, and R3 were on bits the device does not use for those buttons. L2 and R2 stay the analog axes (`Gamepad_LeftTriggerAxis`, `Gamepad_RightTriggerAxis`). The D-pad stays the hat, which emits `Gamepad_DPad_*`.

Menus (front end, mode select, fighter select, pause) now use Game and UI, focus the local player's widget, and read the left stick and D-pad directly. Cross accepts, Circle backs out, and Options toggles pause where pause exists. Opening a menu removes the fight mapping from that local player. Closing it, or the window coming back, puts Game Only and the fight mapping back on that same player. AxisConfig stick dead zones are 0. The left stick has one 0.22 radial zone. The right stick still yaws under 0.55 and punches at or above 0.55. Keyboard keys are unchanged. `docs/Controls_UndisputedCompetitor.md` was not edited. The PlayStation names are the same.

- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (38.16 s) and linked `UnrealEditor-FinalBell.dll`. Live Coding did not block it.
- PIE was not run. The pad was not watched. The events were not seen in a session.

### 2026-10-20 — Fight controls died at the bell

The pad still worked on the menus, and Options still opened pause, because those keys are read on the player controller. Move, punches, block, weave, and the sticks are Enhanced Input actions on the possessed boxer. Opening fighter select or the mode menu removes that mapping. `RestoreFightMapping` put it back only when `FSceneViewport::IsForegroundWindow()` was true. That call asks the viewport HWND if it is the OS foreground window. In the editor that HWND is not, even while the same controller is receiving the pad. The bell therefore set `bFightMappingWanted` false, called `RemoveMappingContext`, and the next tick never restored it because the sample never changed. The round phase was already `RoundActive`. `AllowsNewAttacks` allows new punches during that phase. Movement is not gated on the phase. The actions never fired. The left stick still has one 0.22 radial zone. AxisConfig dead zones stay 0.

Closing the menu or pause now sets Game Only, focuses the game viewport, and adds the fight mapping on the local player who owns the possessed boxer. A lost Slate focus can still remove it. Coming back adds it again. If the confirming player has no boxer when the bell rings, `StartBout` possesses the human fighter first. The pad was not watched. The events were not seen in a session.

- Build: `Build.bat FinalBellEditor Win64 Development -Project FinalBell.uproject -WaitMutex` **Result: Succeeded** (10.87 s) and linked `UnrealEditor-FinalBell.dll`. The first pass failed to compile because a local named `Owner` hid `AActor::Owner`. Live Coding did not block the link.



