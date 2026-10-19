# Final Bell — what the gameplay code does

This is a map of the Unreal C++ in `apps/game-unreal/Source/FinalBell`. Each function is one job. The server owns hits, stamina, and the result. A client only asks to punch, move, or block.

Head damage, body damage, stun, and the waist coil are explained as a reading lesson in [DamageAndWaist.md](DamageAndWaist.md).

A solo fight is two boxers: you, and one opponent. `Ring_P1` also has a boxer saved in the map. `EnforceRoster` removes that extra body when the match starts.

## Combat types — `Combat/FBCombatTypes.h`

Shared names used by input, the server, and the pose.

| Name | What it is |
| --- | --- |
| `EFBStance` | Orthodox or southpaw. Southpaw mirrors which hand is the lead. |
| `EFBPlayMode` | Sparring (AI), multiplayer, heavy bag training, or career. |
| `EFBAttackId` | Jab, straight (`Cross`), lead hook, rear hook, lead uppercut, rear uppercut. |
| `EFBGuardState` | Hands open, high guard, low guard, or weaving. |
| `EFBContactResult` | Miss, blocked, or clean hit. |
| `EFBAttackPhase` | Idle, startup, the hit window, then recovery. |
| `EFBHitRegion` | Head or body. Tells the server which pool to fill, and tells the pose which flinch to play. |
| `EFBCornerSkin` | Blue corner or red corner colors. |
| `FFBCombatIntent` | One request from the player: which punch, whether guard is held, body or power. It does not contain damage. |
| `FFBImpactEvent` | The server's answer: hit or miss, damage, region, and whether a slip or angle bonus happened. |

## The boxer — `Combat/FBBoxerCharacter.h`

The pawn you move. `AFBBoxerCharacter` is both you and the opponent.

| Function | What it does |
| --- | --- |
| `AFBBoxerCharacter` | Builds the capsule, the combat component, the skinned mannequin, and the fallback limb shapes. |
| `GetLifetimeReplicatedProps` | Lists the values the server copies to other machines: stance, colors, head damage, body damage, stun, the stick angle, and the facing lock. |
| `BeginPlay` | Shows the mannequin and paints the gloves in the corner color. |
| `Tick` | Faces the opponent, fades the flinch, fades stun on the server, sets walk speed from the pools, updates the punch pose, and keeps the personal camera pointed. |
| `SetupPlayerInputComponent` | Binds keys and the gamepad to move, look, and punch. |
| `PossessedBy` | When a controller takes this pawn, installs the input and the camera. |
| `PawnClientRestart` | Same setup when a client finishes possessing the pawn. |
| `OnRep_PlayerState` | Required network hook. No extra work. |
| `GetCombatComponent` | The stamina and hit resolver on this fighter. |
| `ApplyAuthoritativeDamage` | Server adds one punch to the head pool or the body pool, and raises stun. A repeated punch id is ignored. Filling a pool does not end the bout. |
| `GetAccumulatedDamage` | Head plus body. The combined total. |
| `GetHeadDamage` | Lasting chin damage. |
| `GetBodyDamage` | Lasting rib damage. This is what slows energy refill. |
| `GetStun` | The wobble after a shot. The server fades it. |
| `GetOutgoingDamageScale` | Multiplier on the puncher's next shot. Body damage and stun lower it. Lasting head damage does not. |
| `GetBurstRecoveryScale` | How fast burst energy refills. Full body damage uses `BodyRecoveryAtFull` (0.45). |
| `PlayHurtReaction` | Starts the flinch on the fighter who was hit. Yaw, fold, and a chin tuck. Presentation only. |
| `ComputeDisplayedPunchYaw` | How many degrees this punch coils, already blended and mirrored for southpaw. |
| `GetMoveSpeedScale` | Walk-speed multiplier from stun and body damage. The capsule stays the same size. |
| `DecayStun` | Server subtracts stun over time. |
| `DecayHurtReaction` | Eases the flinch back to a normal guard on this machine. |
| `ApplyConditionToMovement` | Writes `MaxWalkSpeed` from the base speed and `GetMoveSpeedScale`. |
| `BuildWaistTwist` | Splits a turn across the hips, the lumbar spine, the mid back, and the chest. Lives in `FBBoxerCharacter.cpp`. |
| `ServerSubmitCombatIntent` | Client sends a punch or guard request to the server. |
| `ApplyBasicSkin` | Paints skin, trunks, gloves, and shoes for blue or red. |
| `RefreshCombatPresentation` | Kept so the combat component can notify the body. The pose update does the drawing. |
| `HandleImpactPresentation` | On a clean hit or a block, plays the flinch on the defender and prints the region and the amount. |
| `ApplyPlaceholderAppearance` | Blue for the player, red for the opponent. |
| `GetStandingHalfHeight` | How tall the capsule is, including body scale. Used to stand on the canvas. |
| `GetAnglePowerMultiplier` | 1.5 while this fighter is in the bonus window after slipping a hook. Otherwise 1. |
| `IsHookFacingLocked` | True while a hook's facing is frozen so a slip does not turn the punch with the body. |
| `GetHookLockDirection` | The direction that hook was thrown. |
| `DoesPunchConnect` | True when this punch's sphere overlaps the opponent. Body shots aim lower. |
| `BeginHookFacingLock` | Freezes facing for a hook. |
| `ExtendHookFacingLock` | Keeps that freeze if the defender slips it. |
| `GrantCreatedAngle` | Gives the defender a short power bonus after a slip. |
| `OnRep_CornerSkin` | Repaint when the corner color arrives from the server. |
| `Move` | WASD or the left stick, relative to the camera. |
| `Look` | Mouse or Q and E orbit the camera. |
| `OnPunchStick` | A hard right-stick flick throws one punch from the stick direction. |
| `OnPunchStickReleased` | Allows the next flick. |
| `OnTurnStick` | A gentle right-stick tilt turns the capsule up to 50 degrees. The pose lags the hips so the waist shows that turn. |
| `OnTurnStickReleased` | Shoulders square back up. |
| `OnJab` / `OnCross` / `OnLeadHook` / `OnRearHook` / `OnLeadUppercut` / `OnRearUppercut` | Ask the server to throw that punch. |
| `OnBlockStarted` / `OnBlockCompleted` | High guard while Shift or the right trigger (R2) is held. Stick down, or S, lowers it to a body block for that same hold. |
| `OnWeaveStarted` / `OnWeaveCompleted` | Weave while Ctrl or the left trigger (L2) is held. Forward or neutral rolls under. Back leans away. |
| `OnBodyModStarted` / `OnBodyModCompleted` | The punch aims at the body for the whole time C, L1, or LB is held. A tap shorter than a quarter second, with no punch, rolls left. |
| `OnPowerModStarted` / `OnPowerModCompleted` | The punch is a power shot for the whole time V, R1, or RB is held. A tap with no punch and no dodge rolls right. |
| `SubmitLocalIntent` | Packages the request and either runs it locally (server) or sends it. Prints the punch name on screen. |
| `FindOpponent` | The tagged sparring partner. |
| `FindCombatTarget` | The opponent, or the heavy bag in training. |
| `FindOrbitCamera` | This player's camera. |
| `EnsurePersonalCamera` | Spawns that camera once for the local player. |
| `FaceOpponent` | Turns to face the target unless a hook has locked the facing. |
| `UpdateFacingOffset` | Smooths the stick angle. The owning client sends it to the server when it moves more than 1.5 degrees. |
| `FadeReleasedMove` | After the stick is released, eases the last step down to zero instead of cutting it. |
| `PushSmoothedMove` | Applies the eased stick to the capsule. Power plus the stick is still the faster dodge scale. |
| `UpdateMovementYawLag` | Stores how far the capsule yawed ahead of the hips this frame. |
| `UpdateDefensePose` | Eases slip, roll, and lean. A weave reads the stick on the owner and the velocity on everyone else. |
| `BeginDefenseSlip` | Picks slip left, slip right, roll under, or lean back from the stick. Does not move the capsule. |
| `ConfigureBodyCollision` | Capsule size, weight, and the rule that bodies block each other. |
| `GetSyncedTimeSeconds` | Server clock, so a lock ends at the same time on every machine. |
| `EnsureRuntimeInput` | Creates the key map in memory if this pawn does not have one yet. |
| `ServerSetFacingOffset` | Client tells the server the shoulder angle. |
| `OnRep_AngleBonus` | Shows a short message when the angle bonus arrives. |
| `ApplyMappingContext` | Puts the key map on the local player. |
| `TintMesh` | Paints one shape. Gloves use this. The mannequin keeps its own materials. |
| `TintSkeletalMesh` | Tries tint parameters on a skeletal mesh. The posed mannequin does not use it. |
| `SetupMannequinVisuals` | Puts Manny or Quinn on the pose mesh and hides the cylinder body. Falls back to shapes if the mesh has no limb bones. |
| `TryActivateSkinnedMesh` | Loads the corner mesh and checks that the arm and leg bones exist. |
| `HideShapeBody` | Hides or shows the cylinder body. Gloves stay visible. |
| `AttachGlovesToCapsule` | Parents the gloves to the capsule for the shape fallback. |
| `UpdateDisplayedPunch` | Copies the server punch phase into the pose blend. |
| `UpdateBoxerPose` | Chooses the mannequin pose or the shape pose. |
| `ApplyShapePose` | Places the cylinder limbs in the guard or the punch. Used only when the mannequin did not load. |
| `ApplySkinnedPose` | Writes the guard and the active punch onto the mannequin bones. |
| `AttachGlovesToHands` | Parents the gloves to `hand_l` or `hand_r`. Lead and rear swap in southpaw. |
| `LeadHandSocket` / `RearHandSocket` | Those hand bone names. |

`ApplySkinnedPose` is what makes a punch visible on the mannequin. It starts from the mesh reference pose, sets a boxing stance, then aims each arm at the guard or at the punch. Between punches the chest breathes, the shoulders roll, the head counters that idle roll, and the torso shifts with the step. Jab and lead hook move the lead arm. Straight, rear hook, and rear uppercut move the rear arm. Hold C, L1, or LB and that punch bends the torso and finishes at the ribs. A face button throws that punch by itself. A hard right-stick flick throws the same punch from the direction. Uppercuts are X+Square, Circle+Triangle, the down-left and down-right flicks, and keys 5 and 6. There is no separate animation asset; the pose is rebuilt every frame from the punch phase.

The turn of a punch and of the right stick goes through `BuildWaistTwist`. The hips take a share, `spine_01` (the waist) takes the largest share, then `spine_02`, then `spine_03`. The neck turns back toward the opponent so the chin stays on target. The feet stay on their planted targets. The capsule and the punch spheres do not follow this bone motion. The full reason is in [DamageAndWaist.md](DamageAndWaist.md).

A slip, a roll, and a lean use `BuildDefenseBend` with those same four shares. Positive pitch folds the chest toward the opponent (the roll under, 74°). Negative pitch leans the chest back (58°). Positive roll moves the head to the fighter's left (48° through the trunk). The bend is written on `pelvis`, `spine_01`, `spine_02`, and `spine_03`. `neck_01` gives back 0.40 of it, and `head` keeps 0.22 of the roll, so a side roll leaves the head about 39° off the punch line. The low shoulder drops about 17 cm. The feet stay on the same IK targets. `AuthoritySlip` still launches the capsule and still grants the angle. A tap of L1 or R1 only writes the bones, then the server uses that pose as a head slip. The weave guard still slips a hook on the server. See [Controls_UndisputedCompetitor.md](Controls_UndisputedCompetitor.md).

## Hits and stamina — `Combat/FBCombatComponent.h`

Runs on the server. Clients see the replicated phase and play the pose.

| Function | What it does |
| --- | --- |
| `UFBCombatComponent` | Turns the component tick on and marks it replicated. |
| `GetLifetimeReplicatedProps` | Copies energy, guard, the active punch, and the last impact to clients. |
| `TickComponent` | On the server, advances startup, the hit window, and recovery, and refills energy while idle. Body damage slows that refill. While the hit window is open it keeps asking `ResolveAgainstOpponent` until the glove connects or the window ends. |
| `ServerTryProcessIntent` | Accepts a punch only when idle and there is enough energy. Ignores old or wrong-version requests. |
| `GetBurstEnergy` | Current punch energy. |
| `GetFatigue` | Energy that does not come back as fast. |
| `GetGuardState` | Open, high, low, or weave. Low is the body block while R2 is held and the stick is down. |
| `GetActiveAttack` | The punch in progress. Stays set through recovery so the arm can return. |
| `GetAttackPhase` | Idle, startup, active, or recovery. |
| `IsAttackCommitted` | True from the start of a punch until it is idle again. |
| `IsBodyShot` | This punch is aimed at the body. |
| `IsPowerShot` | This punch uses the power multiplier. |
| `OnRep_CombatVisual` | Client redraws the pose when the phase changes. |
| `OnRep_LastImpact` | Client plays the defender's flinch. |
| `CanAfford` | True if energy covers this punch. |
| `CostFor` | Energy spent by each punch. |
| `DamageFor` | Base damage of each punch, before power, body, block, and angle. |
| `SetPhase` | Switches phase and tells the boxer to redraw. |
| `BeginAttack` | Spends energy and starts startup. Hooks also lock facing. |
| `ResolveAgainstOpponent` | During the hit window: miss, slip, block, or clean hit. Applies damage once, into the head pool or the body pool. The puncher's body damage and stun scale the amount down. A whiff is not final until the window ends. |
| `DoesPunchConnect` | On `AFBBoxerCharacter`. The glove. A sphere in front of the capsule, lower for a body shot. The server runs it. The client never reports a hit. |
| `IsHeldDefenseOnTime` | High guard or weave counts only when that hold began before the glove window opened. A hold raised after the window is late and is not a clean block. Releasing the hold early leaves the guard open. |
| `IsHeadEvasionOnTime` | On `AFBBoxerCharacter`. Slip, roll, and lean. The bone pose beats a head shot only when it started before the window and has not already ended. It does not move the capsule or the punch sphere. A body shot still lands. |
| `DidDefenderRollUnder` | True if the defender stepped far enough off a hook's line. |
| `ApplyAuthoritativeDamage` | On `AFBBoxerCharacter`. The only place a punch is added to head or body. It refuses a repeated punch id and it refuses a client. |
| `NoteImpact` / `JudgePoints` / `BuildDecisionCard` | On `AFBSparringGameMode`. The one score path. Three judges, 10-point must. They score ring generalship, knockdowns, and punch count. A clean punch is worth more punch credit than a block. |

A high guard blocks a head punch when `IsHeldDefenseOnTime` says the guard was already up. It does not block a body punch. A weave slips a hook only when that weave was already held. Stepping off the hook line slips it only when the punch no longer touches the defender.

The server owns the hit, the stamina cost, the guard, the referee, the knockdown, and the card. `FFBCombatIntent` names the punch and the guard. It has no damage field and no winner.

There is no `UPhysicalAnimationComponent`. `ApplySkinnedPose` writes the mannequin bones every frame, including slips and punches. A physics blend on those bones would fight that write. The heavy bag is the physics body. The punch spheres stay on the capsule.

## The match — `Combat/FBSparringGameMode.h`

| Function | What it does |
| --- | --- |
| `AFBSparringGameMode` | Sets the boxer pawn and the sparring player controller. |
| `BeginPlay` | Restores a saved mode, then builds the ring after a short delay. |
| `Tick` | Keeps the roster at two fighters, and runs the round clock. |
| `PostLogin` | Rebuilds the roster when another player connects. |
| `ChoosePlayerStart_Implementation` | Walks the player starts so two humans do not stack. |
| `GetDefaultPawnClassForController_Implementation` | Every player is a boxer. |
| `GetPhase` | Pre-bout, round, rest, or complete. |
| `GetCurrentRound` | The current round number. |
| `StartBout` | Starts round 1. Refuses to run unless the phase is still pre-bout, so a confirmed fight resets the phase first. |
| `ApplyPlayMode` | Switches to sparring, multiplayer, training, or career, clears the previous bout, and rebuilds the ring. Career uses the same one-opponent placement as sparring. |
| `SetAwaitingFighterConfirm` | Holds the arena timer so 1v1 does not start until Fight is accepted. |
| `AbandonBoutForMenu` | Clears knockdown, clinch, clock, and the card without writing a result. |
| `SetPresentationPaused` | Host-only freeze of the bout clock. A remote client cannot set it. |
| `AdvancePhase` | Round, then rest, then the next round, then the end. |
| `FinalizeResultOnce` | Logs the end of the bout a single time. |
| `TryBuildArenaAndStart` | Places fighters and starts the bout if that is enabled. |
| `PlaceFightersForMatch` | Removes extra bodies, builds the ring, spawns one AI opponent only in solo sparring. |
| `DestroyAllAiOpponents` | Deletes every boxer that is not controlled by a player. |
| `EnforceRoster` | Solo sparring and career keep one opponent. Multiplayer and training keep none. |
| `DestroyTrainingBag` | Removes the heavy bag. |
| `CountHumanBoxers` | How many boxers are possessed by players. |
| `EnsureTrainingBag` | Spawns the bag if training needs one. |

## Player controller — `Combat/FBSparringPlayerController.h`

| Function | What it does |
| --- | --- |
| `AFBSparringPlayerController` | Standard controller setup. |
| `BeginPlay` | Shows the mode menu until a mode is chosen. |
| `EndPlay` | Closes that menu. |
| `FBMode` | Console: `FBMode 1` sparring, `2` multiplayer, `3` heavy bag, `4` career. |
| `FBHost` | Starts a listen server. Default port 7777. |
| `FBJoin` | Travels to another host. |
| `FBRoom` | Prints the current room address. |
| `ShowModeMenu` / `HideModeMenu` | The on-screen sparring, multiplayer, heavy bag, and career rows. |
| `ChooseMode` | Sparring and career close the poster and open fighter select. They do not store the mode or start the clock. Career with exactly one ready created fighter starts that bout instead. Multiplayer and the heavy bag apply immediately. |
| `ServerStartCompetitiveBout` | Fight sends the two builds and the mode. The server starts sparring or career only when both cost 600 and share a class. Career also requires a premade opponent. |
| `ShowPauseMenu` | Options or Escape during a bout. Resume, Main Menu, Change Mode, Quit. |

## Input — `Input/FBRuntimeInputLibrary.h`

| Function | What it does |
| --- | --- |
| `EnsureBoxerInput` | Builds the Undisputed Competitor key map in memory. L2 is `Gamepad_LeftTriggerAxis` (weave). R2 is `Gamepad_RightTriggerAxis` (block). A pull past 0.35 is held. Holds use a Down trigger so they stay down for the whole press. Number keys 1–6 are jab, straight, lead hook, rear hook, lead uppercut, rear uppercut. |

`FFBRuntimeInputBundle` is that map plus each action: move, look, punch stick, shoulder turn, the six punches, block, weave, body, and power.

## Viewport — `Input/FBGameViewportClient.h`

Used so two PIE windows can each have their own gamepad.

| Function | What it does |
| --- | --- |
| `SetupInitialLocalPlayer` | Creates the local player and binds a pad. |
| `Tick` | Retries pad focus for a couple of seconds in the editor. |
| `InputKey` / `InputAxis` | In a two-window PIE session, sends each pad to its own window. |
| `RemapControllerInput` | Solo play accepts any pad. Two windows keep pad 0 and pad 1 apart. |
| `BindPieLocalPlayerToGamepad` | PIE window N uses controller N. |
| `RouteGamepadToMatchingPieViewport` | Forwards a pad event to the other PIE window when it belongs there. |
| `EnsurePieGamepadFocus` | Puts keyboard focus on that window's viewport. |

## Camera — `Camera/FBBroadcastCamera.h`

Ringside camera just above the ropes. It looks slightly down at both fighters so their feet and most of the ring stay in frame, and it eases around the apron to keep a profile view.

| Function | What it does |
| --- | --- |
| `AFBBroadcastCamera` | Creates the camera and the mild contrast grade. |
| `Tick` | Dollies along the apron and eases to a profile of both fighters when look input is idle. |
| `BeginPlay` | Finds the two fighters and possesses the view. |
| `SetFighters` | Remembers who is the local fighter and who is the opponent. The first time, snaps to the south side of the ring. |
| `ForcePossessedView` | Makes the owning controller look through this camera. |
| `ForceViewForController` | Same, for a specific controller. |
| `AddOrbitInput` | Mouse or Q and E move which side of the ring the camera is on, and how high it sits. |
| `GetOrbitYaw` | Current side of the ring. |
| `GetOrbitFlatRotation` | That yaw with no pitch. |
| `AutoFindFighters` | Uses the local pawn and the other boxer if `SetFighters` has not run. |
| `ComputeFrame` | Places the camera outside the ropes and aims at the fighters' hips. |
| `SnapToFrame` | Jumps there immediately so the first frame is the wide ringside shot. |

## Ring — `Ring/FBP1ArenaBootstrap.h` and `Ring/FBRingBounds.h`

| Function | What it does |
| --- | --- |
| `EnsureArena` | Builds the canvas, ropes, lights, and bounds if they are missing. Spawns the opponent only when asked. The last argument dresses the room as an old-school gym for 1v1 AI. |
| `EnsureBagRoom` | Heavy bag training only. A bag row north of that gym: floor, walls, beam, movable lights, one punching bag, and visual neighbors. Does not move the bout ring. |
| `DestroyBagRoom` | Removes that shell and its lights when training ends. |
| `SpawnFloor` | Canvas, apron, posts, and four ropes. Around that, either the dark bowl or the gym. |
| `SpawnOldSchoolGym` | Wood floor, brick walls, ceiling, beams, windows, bags, bench, bucket, mirror, and posters. Basic shapes. Outside the ropes. |
| `SpawnLighting` | A key light, a shadowless fill, sky light, and overhead spots. The gym path warms those lights and hangs four bulbs. |
| `SpawnRing` | The invisible box that keeps fighters inside the ropes. |
| `SpawnOpponent` | One red-corner boxer, tagged `FB_P1Opponent`, possessed by an AI controller. Reuses that tag only when the actor is not a player pawn. |
| `SpawnCamera` | One broadcast camera. Each human also gets a personal camera from the pawn. |
| `FindFirstPlayerBoxer` | The possessed pawn, if there is one. |
| `AFBRingBounds` | The actor that owns the playable box. |
| `ConfigurePlayableArea` | Sets how far from center a fighter may stand. |
| `ClampToRing` | Pulls a position back inside that box. |

## Training bag — `Training/FBHeavyBag.h`

| Function | What it does |
| --- | --- |
| `AFBHeavyBag` | A leather cylinder 46 cm across and 132 cm tall, with end caps and bands, on a 34 cm chain. Neighbors use the same shape and do not simulate. |
| `BeginPlay` | Loads the engine cylinder and forces that scale. The active bag is a 40 kg body hung from the top. Swing and twist are limited. An angular drive returns it. It overlaps the pawn, so it does not push the capsule. |
| `Tick` | Lays the chain from the beam to the top of the bag. Restores the leather scale if physics collapses it. |
| `ApplyStrike` | Server only. Adds an impulse at the punch contact so the bag swings and twists. It does not resize punch spheres. |
| `OverlapsPunch` | True when the existing punch sphere overlaps the leather. |
| `GetStrikeCenter` | The bag mesh's current location. |

Open the drill from the front-end **Heavy Bag Training** row, or from the sparring mode menu of the same name, or with console `FBMode 3`. That is `EFBPlayMode::Training`. It does not start the 3:00 clock or knockdowns. The fighter stands in the bag room north of the bout gym, not inside the 20 ft ring.

## Fight meters — `UI/FBFightMeters.h`

Shown by `AFBSparringPlayerController` after the mode menu closes. The bars copy replicated numbers. They do not apply damage or spend stamina.

| Function | What it does |
| --- | --- |
| `Construct` | Builds the two corner panels. Left is you. Right is the other fighter. |
| `MakePanel` | One panel: name, stamina, head, body, stun. |
| `MakeRow` | A label, a track, a fill, and the number. |
| `ApplyMeter` | Sets the fill width and the rounded number from a current value and a cap. |
| `Refresh` | Reads both boxers and paints the bars. |
| `UpdateFightMeters` | Adds the widget on the local controller and refreshes it every tick. |
| `RemoveFightMeters` | Takes the widget off the screen when the mode menu or the front end is up. |

Stamina is burst energy, gold, and it empties as you punch. Head, body, and stun fill as those pools rise. The colors are the broadcast set in `Tools/AI/config/design_system.json`. Body is a leather brown and stun is a pale gold so the three pools stay distinct. Oswald is named in that file and is not imported, so the engine font is the stand-in.

## The bout — clock, knockdown, judges

Reading notes: `docs/BoutAndScorecard.md`. The server decides the count and the winner. The overlay only draws them.

| Function | What it does |
| --- | --- |
| `AFBSparringGameState` | The copy of the clock, the count, the two target angles, the downed fighter, and the card that every screen can read. The game mode itself does not replicate. |
| `PushBoutState` | Writes those fields from the server mode. Training leaves the clock stopped. |
| `StartBout` | Starts one 3:00 round and clears the ledgers and the card. |
| `Tick` | Runs the 3:00, or the count of 10 while someone is down. The bout clock waits during the count. |
| `NotifyEnergyDepleted` | Starts a knockdown when a live fighter spends the gas tank to empty. Ignored in training, before the bell, and after the bout. |
| `BeginKnockdown` | Puts that fighter on the canvas, starts the count at 10, and cuts the camera to the referee. |
| `SubmitGetUpAim` | Scores both sticks against the rotating marks. A short hold on both marks beats the count. |
| `CompleteGetUp` | Stands the fighter up and gives back enough energy to box. |
| `FinishKnockout` | Names the other fighter the winner when the count expires. |
| `NoteImpact` | Adds punch credit to the attacker's corner. A clean shot is 1. A block is 0.25. Head and body damage are still recorded and are not judge points. |
| `TickRingGeneralship` | Each second of the 3:00, scores who is at the range their style wants, who forced the other fighter off that range, and who is closer to the center. |
| `BuildDecisionCard` | Three judges, 10-point must, only if both fighters are still in at the bell. Harris leans on punch count. Cole leans on ring generalship. Reed leans on knockdowns. The card shows those three pillars. |
| `AFBBoxerAIController` | Chooses the opponent's next step and the next `FFBCombatIntent`. The goal is to win under the rules: land clean, avoid damage, keep enough energy to stay up, and do not punch a downed fighter. |
| `BuildKnockoutCard` | A knockout card. It does not invent round scores. |
| `ShowReferee` / `HideReferee` | A referee built from basic shapes, with no collision, tagged `FB_Referee`. |
| `SetRefereeCameras` | Low canvas shot looking up at the referee. |
| `SetKnockedDown` | Server flag. The mesh lies down. The capsule stays the same size. |
| `AbortAttack` | Cancels the punch that emptied the tank so it cannot land from the canvas. |
| `RestoreBurstEnergy` | Puts energy back after a successful get-up. |
| `SFBBoutOverlay::Refresh` | Draws the clock, the two circles, and the card. |
| `SubmitLocalGetUpAim` | The downed local fighter sends the sticks. The listen server scores them directly. A client uses `ServerSubmitGetUpAim`. |

## How each style fights

`FFBStylePolicy::ForStyle` is the small policy. `AFBBoxerAIController` runs it only in sparring and career, and only while the 3:00 is live. It submits `FFBCombatIntent` through `ServerTryProcessIntent`. Block, slip, weave, and lean use that same path. The accuracy rating is the chance the intent sets the existing precision flag. Feet, hands, guard, and stamina stay on the ratings, so a slow slugger does not grow a second set of feet. A created fighter with no style name, or the word Custom, uses the boxer-puncher mix. The heavy bag does not.

| Style | What they do |
| --- | --- |
| Slugger | Walks in and looks for one heavy rear hand. Does not circle, and does not try to win a foot race. |
| Power puncher | Plants for a heavy shot from either hand. The gas and the feet are the limit. |
| Out-boxer | Keeps long range and jabs. Steps out instead of brawling inside. |
| Slick boxer | Circles off the line and jabs. Does not stand in the pocket. |
| Jab technician | The jab is the fight. Leaves when the other fighter gets inside. |
| In-fighter | Closes, works the body, and gets hit because the guard and the feet are the weakness. |
| Swarmer | Smothers at short range and keeps the hands going. The chin and the guard pay for it. |
| Inside fighter | Cuts the ring off and stays on the body. |
| Brawler | Walks through punches and swings inside. Does not chase a mover around the ring. |
| Counterpuncher | Waits for a miss or a block, then answers. Does not lead every exchange. |
| High guard | Blocks first, then counters from the shell. Does not chase. |
| Peek-a-boo | Shells up, then a short counter. Does not pick a fight from across the ring. |
| Southpaw | Steps to the left-hand angle and fights off the rear hand. |
| Body puncher | Once in range, the punches go to the body. |
| Boxer-puncher | Mixes the jab and the power shot at mid range. Does not become a slugger or an out-boxer. |
| Speedster | Throws more punches, does not load up power, and does not stand in the pocket. |

## Damage you can see, and the Competitor punches

Reading notes: `docs/Controls_UndisputedCompetitor.md`. Decision D-023. D-015 still covers fouls and the cosmetic damage meshes.

| Function | What it does |
| --- | --- |
| `EnsureDamageMarkers` | Builds the cheek bruise, the swollen eye, the bloody nose, the brow cut, and the rib bruise. No collision. |
| `ApplyVisibleDamage` | Grows those marks from the replicated head and body pools. The head and body shapes darken with them. |
| `GetActionTimeScale` | Slows the next punch when the body pool is high. Full body damage is about 1.85 times slower. |
| `GetMoveSpeedScale` | Slows the step. Full body damage takes 55% of the walk speed off, down to about a third. |
| `MatchPrecisionAttack` | Picks the punch from the face buttons that are held. Square is the jab, Triangle the straight, X the lead hook, Circle the rear hook. X+Square and Circle+Triangle are the uppercuts. Southpaw swaps lead and rear. |
| `TryStickPunch` | A hard right-stick flick, or the arrow keys, throws that same punch from the direction. Straight down is not a punch. |
| `TryPrecisionPunch` | Throws the button punch. The stick cone is a 6% bonus when it agrees. It is not required. |
| `HandleRingAction` | Server: push, headbutt, low blow, clinch, slip, stance, loose movement, taunt. |
| `TickClinch` | Holds the two fighters together. Breaks it after about 4.6 seconds. |
| `AddClinchSteer` | Fills the turn meter from the left stick. At 100 the opponent is spun and the turn count goes up. |
| `DeductPoint` | Takes a point off that corner on every judge's card. A second foul, or a third clinch, is what spends it. |

## CompetitiveFair builds

Reading notes: `docs/FighterBalance.md`. Decision D-016.

| Function | What it does |
| --- | --- |
| `FFBFighterBalance::CostOfRating` | The only point cost. 50 is free. 51–70 cost 1, 71–80 cost 2, 81–90 cost 3, 91–95 cost 5. |
| `FFBFighterBalance::Derive` | Turns a legal rating list into the combat multipliers. At 50 they are all 1. |
| `FFBFighterBalance::IsReady` | True only when the build costs exactly 600. |
| `ApplyFighterRecord` | The server copies a ready build onto the boxer. An illegal build is refused. The capsule stays the same size. |
| `TakeRegionalDamage` | Applies chin or body resistance once, then a small cut or bruise consequence. |
| `NoteBlockedImpact` | Spends block health. Regen waits 0.8 seconds. |
| `GetAccuracyCone` | Tight stick cone, about 26 degrees at rating 70. A face button or key still throws if the stick is outside it. Inside the cone is a 6% damage bonus. |
| `MatchPrecisionAttack` | Square/J/1 jab, Triangle/K/2 straight, X/U/3 lead hook, Circle/I/4 rear hook, Y/5 and X+Square lead uppercut, H/6 and Circle+Triangle rear uppercut. A hard stick flick throws the same punch. |
| `ServerStartCompetitiveBout` | The client proposes two builds and sparring or career. The server calls `ApplyPlayMode` for that mode, which places the player's pawn and one AI opponent, possesses that opponent, and calls `StartBout`. An illegal pair stays on the fighter screen with the reason. A second PIE client does not delete that AI or stand in for the selected premade. Its pawn is benched, so 1v1 vs AI is the local human and that one opponent. Multiplayer still keeps both humans and does not spawn the AI. |
| `AwardCareerFromCard` | When a bout card is finalized, a created fighter in that bout gets +25 Career XP, a win, loss, or draw, and 20 retrain points. The ratings themselves are not raised. The write is the local `FBFighterRoster` save. It is not an official ranked pool. |

## Create a Fighter

The screen is the 1v1 Slate builder. Every field below is stored on `FFBPresentation` inside the `FBFighterRoster` save. The live fighter stays the mannequin. Height, chest, waist, and the other body numbers do not change the capsule or the punch spheres.

In the fight you can see: skin tint from skin U/V, hair and beard proxies, gray hair, tattoo placement and density, a scar mark, glove color, and glove size from muscularity. The pawn turns slowly while the builder is open.

Editor only, because this install builds a MetaHuman through `UMetaHumanCharacterEditorSubsystem` plus an Epic texture and rig download: the MetaHuman mesh itself, face-model coefficients, body-constraint solve, iris texture, eyelash groom, teeth mesh, makeup materials, and clothing wardrobe items. The saved numbers are the ones to type into that editor later. No age or wrinkle slider exists on this plugin. Skin tone is the skin map U and V. Freckles are None, Type 1, Type 2, Type 3.

The 2026-10-19 15:17 startup log does not ask the project for a MetaHuman asset. MetaHuman Character reports `MetaHuman Optional Content folder not found`, and the MetaHuman Generator startup script then says the optional content is not available. That pack is Epic's download for the Creator plugin. The enabled plugins stay enabled. There is no `MH_*.uasset` in the project, and `create_metahuman_roster.py` has not produced one. That is separate from the startup lines.

| Page | Controls the player sets |
| --- | --- |
| Identity | Nickname, division, stance, country, masculine/feminine, glove, trunks, stripe, shoes, hand wraps |
| Body | Height, muscularity, fat, masculine/feminine, chest, waist, hip, bicep, thigh |
| Face | Brow, eyes, nose, cheeks, jaw, chin, lips, ears, asymmetry. These move marks on the mannequin. MetaHuman sculpts the same regions with landmarks, not these named sliders. |
| Skin | Skin U, skin V, freckle mask, freckle density, iris 1–9, iris hue, foundation, blush, lip makeup, tooth length |
| Hair | Bald/crop/short/fade/braids, gray, beard, mustache, eyelash type, tattoo place and density, scar, groom slot (Hair, Eyebrows, Beard, Mustache, Eyelashes, Peachfuzz) |
| Ratings | The 19 CompetitiveFair ratings, next-point cost, and the Balanced, Puncher, Counter, and Pressure presets |

Randomize changes the look and leaves the 600-point ratings alone. Start-from copies one of the sixteen division fighters into a new custom. A bout still needs exactly 600 points and the same weight class. Ten created fighters is the save limit. Premades do not count toward that cap.

## Front-end menu — `UI/FBMenuWidget.h`

The poster is both the front-end menu and the sparring mode menu. `UFBMenuWidget` draws it for a packaged launch and for editor PIE with `-ShowMenu`. `AFBSparringPlayerController::ShowModeMenu` draws the same poster when PIE opens on the ring. Editor PIE still skips the front end unless `-ShowMenu`. `-SkipMenu` still keeps a packaged build on the ring.

The photo is `Content/UI/FB_MenuArena.jpg`. Its left side is flat black, so the source picture's words are not in the file. The menu draws **FINAL** / **BELL** and the rows on that side. The selected row is a red bar with a white tick. Create a Fighter is not its own row. It opens after 1v1 Sparring (AI), and after 1v1 Career when the save does not hold exactly one ready created fighter.

| Shown label | What it does |
| --- | --- |
| `1V1 SPARRING (AI)` | Travels to the ring. The arena photo stays, and the fighter list slides in over it. Create a Fighter stays on that screen. Fight still waits for two legal 600-point builds in one class. |
| `1V1 MULTIPLAYER` | Travels and applies multiplayer. It does not open fighter select. |
| `HEAVY BAG TRAINING` | `EFBPlayMode::Training` against `AFBHeavyBag`. It does not start the 3:00 clock. |
| `1V1 CAREER` | One 3:00 bout, same server rules as sparring. The fighter list always opens. The player picks their fighter and a premade in that class group. The card writes wins, losses, draws, and +25 Career XP on that created fighter in `FBFighterRoster`. |
| `SETTINGS` | Volume, display, and reduced motion. Front-end only. Not a game mode. |
| `CREDITS` | Project title, studio, and copyright. Front-end only. Not a game mode. |
| `QUIT GAME` | Asks, then closes the game. Front-end only. Not a game mode. |

The sparring poster and pause **Change Mode** list the first four. Pause itself stays Resume, Main Menu, Change Mode, Quit. Console `FBMode 1` through `4` match those four. D-pad, left stick, Cross, and Circle move the new row the same way as the others.

## Fighter select

1v1 Sparring (AI) and 1v1 Career both open this screen. The old full-screen slate titled CHOOSE YOUR FIGHTER, with YOU / THEM prefixes and an OPP button on every row, is gone. Choosing either mode removes the poster and slides this screen in over the same arena photo (about 0.36s, or instantly when reduced motion is on). It does not start the bout. Fight calls `ConfirmCompetitiveBout`, then `ServerStartCompetitiveBout`. The screen stays up until the server accepts. Acceptance hides the screen, sets `FInputModeGameOnly`, points the ringside camera at the two boxers, and `StartBout` runs a fresh pre-bout. A refusal leaves the screen up and prints the reason in red. Back calls `HideFighterSelect` and opens the poster again.

The screen is two dark panels over that photo. The left panel is the player's fighter. The right panel is the opponent. Each panel shows the mannequin stand-in (the skin, hair, trunks, and gloves already stored on the fighter; there is no portrait texture and no MetaHuman), the name, the division, the style, and all 19 CompetitiveFair ratings. The names are `FFBFighterBalance::AttributeName` in `EFBAttributeId` order, which is the order stored on the fighter record. The numbers are that record's ratings. The two low ratings for the style are marked in red. Under each card, Change Fighter lists the sixteen premades plus any legal created fighter, and Change Opponent lists only ready fighters in the same class group (bantam, feather, and lightweight; welter, middle, and light heavy; cruiser and heavy). Career keeps the opponent list to premades. Next Opponent moves only through that list. Two premades share each division, with different styles. Cross on a row changes that side. Create a Fighter still opens the 600-point builder, and the ten-save cap still ignores premades.

Each panel has a Keys to winning block aimed at the other fighter. The left block says how to approach the opponent's style. The right block says how that opponent's style tries to beat the player's style. `KeysToApproach` and `KeysTheyUse` in `Combat/FBFighterSelect.cpp` pick the lines from the roster style: slugger, brawler, in-fighter, jab technician, counterpuncher, power puncher, boxer-puncher, body puncher, swarmer, slick boxer, out-boxer, inside fighter, high guard, speedster, southpaw, peek-a-boo. A style outside that list uses the two lowest ratings on the record. Each block names those low ratings in plain language. The 600-point numbers are unchanged.

The red frame is the focused panel. The red bar is the focused row. D-pad, left stick, and WASD move the row from the player's list, through the opponent's list, to Fight, Back, and Create a Fighter. Cross, Enter, or Space activates it. Circle or Escape backs out. With the menu closed, those face buttons are punches again. Options opens and closes pause. Shoulders and the right stick stay combat inputs during the bout.

Fight calls `ConfirmCompetitiveBout`, then `ServerStartCompetitiveBout`. A legal pair spawns the player's boxer and one tagged AI opponent, possesses that opponent, applies both builds, and `StartBout` runs from pre-bout. Closing the screen hides the widget and sets `FInputModeGameOnly`. It does not destroy the broadcast camera or the opponent. Two PIE clients no longer flip sparring to multiplayer or delete that AI. That delete was the "opponent did not enter the ring" refusal. An illegal pair stays on the screen with the budget or class reason.

Leon Voss is a southpaw whose style is the counter, not the stance. Rafi Solano is the southpaw build (rear left hand). Diego Marte is a southpaw swarmer, separate from Ivan Petrov's in-fighter. Andre Pell is a southpaw body puncher. Luis Ortega is a southpaw inside fighter.

| Fighter | Division | Style | High | Low |
| --- | --- | --- | --- | --- |
| Marcus Hale | Heavyweight | Slugger | Right hand 95, left hand 90, chin 88 | Feet 52, guard recovery 52 |
| Caleb Drummond | Heavyweight | Brawler | Courage 93, chin 90, body 90 | Feet 51, accuracy 52 |
| Ivan Petrov | Cruiserweight | In-fighter | Conditioning 92, endurance 90, body 88 | Feet 56, reflexes 58 |
| Idris Crowe | Cruiserweight | Jab technician | Reflexes 93, left hand 90, accuracy 90 | Body 51, conditioning 52 |
| Leon Voss | Light heavyweight | Counterpuncher | Reflexes 93, accuracy 92, guard 86 | Stamina 55, endurance 56 |
| Mateo Ruiz | Light heavyweight | Power puncher | Chin 93, left hand 90, right hand 90 | Stamina 51, feet 52 |
| Samir Quinn | Middleweight | Boxer-puncher | Left hand 86, right hand 86, accuracy 84 | Guard recovery 64, conditioning 66 |
| Andre Pell | Middleweight | Body puncher | Endurance 93, body 90, conditioning 90 | Chin 51, reflexes 52 |
| Diego Marte | Welterweight | Swarmer | Hand speed 92, endurance 90, stamina 88 | Chin 54, guard recovery 55 |
| Chris Adel | Welterweight | Slick boxer | Accuracy 93, reflexes 90, feet 90 | Right hand 51, body 52 |
| Ellis Kane | Lightweight | Out-boxer | Feet 94, accuracy 91, hand speed 90 | Right hand 54, chin 56 |
| Luis Ortega | Lightweight | Inside fighter | Endurance 93, body 90, conditioning 90 | Feet 51, reflexes 52 |
| Nico Abebe | Featherweight | High guard | Guard 94, block 92, guard recovery 90 | Feet 54, hand speed 56 |
| Kenji Sato | Featherweight | Speedster | Reflexes 93, feet 90, hand speed 90 | Chin 51, right hand 52 |
| Rafi Solano | Bantamweight | Southpaw | Left hand 93, feet 88, accuracy 86 | Bruise 55, guard recovery 57 |
| Pavel Nemec | Bantamweight | Peek-a-boo | Courage 93, guard 90, block 90 | Feet 51, accuracy 52 |

## Session and rooms

`Session/FBSessionSubsystem.h` remembers the mode you picked so it survives a map reload.

| Function | What it does |
| --- | --- |
| `ChoosePlayMode` | Saves sparring, multiplayer, training, or career. |
| `GetPlayMode` | The saved mode. |
| `HasChosenPlayMode` | False until the menu or `FBMode` has been used. |

`Network/FBPrivateRoomSubsystem.h` is the private-room helper. It is not public matchmaking.

| Function | What it does |
| --- | --- |
| `CreatePrivateRoom` | Makes a room code and stores the host address. |
| `SetHostAddress` | Overrides the address you share. |
| `HostListenServer` | Restarts the current map as a listen server. |
| `JoinByAddress` | Travels to `host:port`. |
| `JoinPrivateRoom` | Looks up a code. P1 travel still needs the address. |
| `GetActiveRoom` | The room record. |
| `GetSessionTransportNotes` | Short text about how joining works right now. |
| `GetPreferredHostAddress` | A non-loopback IPv4 address when one exists. |
| `GenerateRoomCode` | A short code for the room. |
| `NormalizeHostPort` | Splits `host:port` into the two pieces travel needs. |
