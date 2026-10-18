# Final Bell — what the gameplay code does

This is a map of the Unreal C++ in `apps/game-unreal/Source/FinalBell`. Each function is one job. The server owns hits, stamina, and the result. A client only asks to punch, move, or block.

A solo fight is two boxers: you, and one opponent. `Ring_P1` also has a boxer saved in the map. `EnforceRoster` removes that extra body when the match starts.

## Combat types — `Combat/FBCombatTypes.h`

Shared names used by input, the server, and the pose.

| Name | What it is |
| --- | --- |
| `EFBStance` | Orthodox or southpaw. Southpaw mirrors which hand is the lead. |
| `EFBPlayMode` | Versus AI, multiplayer, or training. |
| `EFBAttackId` | Jab, straight (`Cross`), lead hook, rear hook, lead uppercut, rear uppercut. |
| `EFBGuardState` | Hands open, high guard, low guard, or weaving. |
| `EFBContactResult` | Miss, blocked, or clean hit. |
| `EFBAttackPhase` | Idle, startup, the hit window, then recovery. |
| `EFBCornerSkin` | Blue corner or red corner colors. |
| `FFBCombatIntent` | One request from the player: which punch, whether guard is held, body or power. It does not contain damage. |
| `FFBImpactEvent` | The server's answer: hit or miss, damage, and whether a slip or angle bonus happened. |

## The boxer — `Combat/FBBoxerCharacter.h`

The pawn you move. `AFBBoxerCharacter` is both you and the opponent.

| Function | What it does |
| --- | --- |
| `AFBBoxerCharacter` | Builds the capsule, the combat component, and the visible limbs. Hides the skeletal mesh. |
| `GetLifetimeReplicatedProps` | Lists the values the server copies to other machines: stance, colors, damage, facing lock. |
| `BeginPlay` | Shows the limb body and paints the corner color. |
| `Tick` | Faces the opponent, updates the punch pose, and keeps the personal camera pointed. |
| `SetupPlayerInputComponent` | Binds keys and the gamepad to move, look, and punch. |
| `PossessedBy` | When a controller takes this pawn, installs the input and the camera. |
| `PawnClientRestart` | Same setup when a client finishes possessing the pawn. |
| `OnRep_PlayerState` | Required network hook. No extra work. |
| `GetCombatComponent` | The stamina and hit resolver on this fighter. |
| `ApplyAuthoritativeDamage` | Server adds damage once per punch id. |
| `GetAccumulatedDamage` | Total damage this fighter has taken. |
| `ServerSubmitCombatIntent` | Client sends a punch or guard request to the server. |
| `ApplyBasicSkin` | Paints skin, trunks, gloves, and shoes for blue or red. |
| `RefreshCombatPresentation` | Kept so the combat component can notify the body. The pose update does the drawing. |
| `HandleImpactPresentation` | Flashes the head on a clean hit or a block. |
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
| `OnTurnStick` | A gentle right-stick tilt turns the shoulders, up to 50 degrees. |
| `OnTurnStickReleased` | Shoulders square back up. |
| `OnJab` / `OnCross` / `OnLeadHook` / `OnRearHook` / `OnLeadUppercut` / `OnRearUppercut` | Ask the server to throw that punch. |
| `OnBlockStarted` / `OnBlockCompleted` | High guard while Shift or R2 is held. |
| `OnWeaveStarted` / `OnWeaveCompleted` | Duck while Ctrl or L2 is held. |
| `OnBodyModStarted` / `OnBodyModCompleted` | The next punch aims at the body while C or LB is held. |
| `OnPowerModStarted` / `OnPowerModCompleted` | The next punch is a power shot while V or RB is held. |
| `SubmitLocalIntent` | Packages the request and either runs it locally (server) or sends it. Prints the punch name on screen. |
| `ResolveUppercutVariant` | LB plus a hook button throws the uppercut on that same hand. |
| `FindOpponent` | The tagged sparring partner. |
| `FindCombatTarget` | The opponent, or the heavy bag in training. |
| `FindOrbitCamera` | This player's camera. |
| `EnsurePersonalCamera` | Spawns that camera once for the local player. |
| `FaceOpponent` | Turns to face the target unless a hook has locked the facing. |
| `UpdateFacingOffset` | Smooths the shoulder angle from the right stick. |
| `ConfigureBodyCollision` | Capsule size, weight, and the rule that bodies block each other. |
| `GetSyncedTimeSeconds` | Server clock, so a lock ends at the same time on every machine. |
| `EnsureRuntimeInput` | Creates the key map in memory if this pawn does not have one yet. |
| `ServerSetFacingOffset` | Client tells the server the shoulder angle. |
| `OnRep_AngleBonus` | Shows a short message when the angle bonus arrives. |
| `ApplyMappingContext` | Puts the key map on the local player. |
| `TintMesh` | Paints one shape. |
| `TintSkeletalMesh` | Unused while the mannequin stays hidden. |
| `SetupMannequinVisuals` | Hides the skeleton and shows the limb shapes. |
| `UpdateBoxerPose` | Places the arms and legs in the guard, and extends the arm that is punching. |
| `AttachGlovesToHands` | Old mannequin glove attach. The pose places the gloves now. |
| `LeadHandSocket` / `RearHandSocket` | Bone names for a future skinned mesh. Lead and rear swap in southpaw. |

`UpdateBoxerPose` is what makes a punch visible. Jab and lead hook move the lead arm. Straight, rear hook, and rear uppercut move the rear arm. Hold C and the same punch drops to the body.

## Hits and stamina — `Combat/FBCombatComponent.h`

Runs on the server. Clients see the replicated phase and play the pose.

| Function | What it does |
| --- | --- |
| `UFBCombatComponent` | Turns the component tick on and marks it replicated. |
| `GetLifetimeReplicatedProps` | Copies energy, guard, the active punch, and the last impact to clients. |
| `TickComponent` | On the server, advances startup, the hit window, and recovery, and refills energy while idle. |
| `ServerTryProcessIntent` | Accepts a punch only when idle and there is enough energy. Ignores old or wrong-version requests. |
| `GetBurstEnergy` | Current punch energy. |
| `GetFatigue` | Energy that does not come back as fast. |
| `GetGuardState` | Open, high, or weave. |
| `GetActiveAttack` | The punch in progress. Stays set through recovery so the arm can return. |
| `GetAttackPhase` | Idle, startup, active, or recovery. |
| `IsAttackCommitted` | True from the start of a punch until it is idle again. |
| `IsBodyShot` | This punch is aimed at the body. |
| `IsPowerShot` | This punch uses the power multiplier. |
| `OnRep_CombatVisual` | Client redraws the pose when the phase changes. |
| `OnRep_LastImpact` | Client plays the hit flash. |
| `CanAfford` | True if energy covers this punch. |
| `CostFor` | Energy spent by each punch. |
| `DamageFor` | Base damage of each punch, before power, body, block, and angle. |
| `SetPhase` | Switches phase and tells the boxer to redraw. |
| `BeginAttack` | Spends energy and starts startup. Hooks also lock facing. |
| `ResolveAgainstOpponent` | During the hit window: miss, slip, block, or clean hit. Applies damage once. |
| `DidDefenderRollUnder` | True if the defender stepped far enough off a hook's line. |

A high guard blocks a head punch. It does not block a body punch. A weave slips a hook. Stepping off the hook line slips it only when the punch no longer touches the defender.

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
| `StartBout` | Starts round 1. |
| `ApplyPlayMode` | Switches to AI, multiplayer, or training and rebuilds the ring. |
| `AdvancePhase` | Round, then rest, then the next round, then the end. |
| `FinalizeResultOnce` | Logs the end of the bout a single time. |
| `TryBuildArenaAndStart` | Places fighters and starts the bout if that is enabled. |
| `PlaceFightersForMatch` | Removes extra bodies, builds the ring, spawns one AI opponent only in solo sparring. |
| `DestroyAllAiOpponents` | Deletes every boxer that is not controlled by a player. |
| `EnforceRoster` | Solo sparring keeps one opponent. Multiplayer and training keep none. |
| `DestroyTrainingBag` | Removes the heavy bag. |
| `CountHumanBoxers` | How many boxers are possessed by players. |
| `EnsureTrainingBag` | Spawns the bag if training needs one. |

## Player controller — `Combat/FBSparringPlayerController.h`

| Function | What it does |
| --- | --- |
| `AFBSparringPlayerController` | Standard controller setup. |
| `BeginPlay` | Shows the mode menu until a mode is chosen. |
| `EndPlay` | Closes that menu. |
| `FBMode` | Console: `FBMode 1` AI, `2` multiplayer, `3` training. |
| `FBHost` | Starts a listen server. Default port 7777. |
| `FBJoin` | Travels to another host. |
| `FBRoom` | Prints the current room address. |
| `ShowModeMenu` / `HideModeMenu` | The on-screen 1v1 AI / multiplayer / training buttons. |
| `ChooseMode` | Stores the choice and tells the game mode. |

## Input — `Input/FBRuntimeInputLibrary.h`

| Function | What it does |
| --- | --- |
| `EnsureBoxerInput` | Builds the key map in memory. Number keys 1–6 are jab, straight, lead hook, rear hook, lead uppercut, rear uppercut. |

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

Chest-high camera behind the local fighter, off the lead-hand side, looking at the opponent.

| Function | What it does |
| --- | --- |
| `AFBBroadcastCamera` | Creates the camera and the mild contrast grade. |
| `Tick` | Follows the fighter and eases back behind them when the stick is idle. |
| `BeginPlay` | Finds the two fighters and possesses the view. |
| `SetFighters` | Remembers who is the local fighter and who is the opponent. The first time, snaps behind the local fighter. |
| `ForcePossessedView` | Makes the owning controller look through this camera. |
| `ForceViewForController` | Same, for a specific controller. |
| `AddOrbitInput` | Mouse or Q and E yaw and pitch. |
| `GetOrbitYaw` | Current yaw around the fighter. |
| `GetOrbitFlatRotation` | That yaw with no pitch, used so movement matches the camera. |
| `AutoFindFighters` | Uses the local pawn and the other boxer if `SetFighters` has not run. |
| `ComputeFrame` | Desired camera position and the point on the opponent it looks at. |
| `SnapToFrame` | Jumps there immediately so the first frame is not a wide shot. |

## Ring — `Ring/FBP1ArenaBootstrap.h` and `Ring/FBRingBounds.h`

| Function | What it does |
| --- | --- |
| `EnsureArena` | Builds the canvas, ropes, lights, and bounds if they are missing. Spawns the opponent only when asked. |
| `SpawnFloor` | Canvas, apron, posts, four ropes, and the bowl around the ring. |
| `SpawnLighting` | A key light, a shadowless fill, sky light, and overhead spots. |
| `SpawnRing` | The invisible box that keeps fighters inside the ropes. |
| `SpawnOpponent` | One red-corner boxer, tagged `FB_P1Opponent`. Reuses that tag if it already exists. |
| `SpawnCamera` | One broadcast camera. Each human also gets a personal camera from the pawn. |
| `FindFirstPlayerBoxer` | The possessed pawn, if there is one. |
| `AFBRingBounds` | The actor that owns the playable box. |
| `ConfigurePlayableArea` | Sets how far from center a fighter may stand. |
| `ClampToRing` | Pulls a position back inside that box. |

## Training bag — `Training/FBHeavyBag.h`

| Function | What it does |
| --- | --- |
| `AFBHeavyBag` | Builds the bag and the chain. |
| `BeginPlay` | Starts the sway simulation. |
| `Tick` | Springs the bag back to center. |
| `ApplyStrike` | Pushes the bag away from the puncher. |
| `GetStrikeCenter` | The point punch range is measured against. |

## Session and rooms

`Session/FBSessionSubsystem.h` remembers the mode you picked so it survives a map reload.

| Function | What it does |
| --- | --- |
| `ChoosePlayMode` | Saves AI, multiplayer, or training. |
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
