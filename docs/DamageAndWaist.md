# Damage and the waist

This page explains two systems that live on the boxer: where a punch's damage goes, and why the torso turns through the waist. The function map is `docs/CodeGuide.md`. The code is `Combat/FBBoxerCharacter.h`, `Combat/FBBoxerCharacter.cpp`, `Combat/FBCombatComponent.cpp`, and `Combat/FBCombatTypes.h`.

The server decides the numbers. A client asks to punch. The pose is a drawing of that result. The drawing does not move the capsule or the punch spheres.

A flash knockout is not in this system. Reaching 100 head damage or 100 body damage does not end the bout. Facial swelling, cuts, and blood are a later art pass. They stay separate from these numbers so a cosmetic change cannot change who won.

## Why the waist was stiff

A real cross or hook turns in this order: the feet stay down, the hips turn a little into the punch, the waist (the lumbar spine) takes most of the turn, the chest finishes it, and the head stays pointed at the other fighter.

The pose was doing something else. Every degree of the punch sat on `spine_03`, which is the upper back, just under the neck. `spine_01`, the bone at the waist, received lean and a shoulder roll, and no yaw. The pelvis was also rotated against the punch, so the hips fought the chest. From the side the shoulders spun and the belt line stayed square.

The right stick had the same problem in a different place. It yawed the whole capsule. Hips, waist, and shoulders turned as one piece, so there was no waist to see.

## How the waist turns now

`BuildWaistTwist` in `FBBoxerCharacter.cpp` splits one turn into four bone yaws. `ApplySkinnedPose` writes them, from the hips upward. Each bone's yaw stacks on its parent, which is how a spine actually bends.

| Bone | Role | Share of the punch | Share of the stick |
| --- | --- | --- | --- |
| `pelvis` | Hips lead the punch. They lag the stick. | 0.22 into the punch | 0.75 back toward the opponent |
| `spine_01` | The waist. This is the turn you should see at the belt. | 0.40 | 0.40 |
| `spine_02` | Mid back, when the mesh has that bone. | 0.24 | 0.22 |
| `spine_03` | Upper back. Finishes the shoulders. | 0.14 | 0.13 |

The punch shares add up to 1. The stick shares on the three spine bones add up to 0.75, which cancels the hip lag. The shoulders end up pointing the same way as the capsule. The punch sphere is attached to the capsule, so the glove and the hit stay lined up. The feet are solved after the hips, onto fixed targets under the capsule, so the legs show the twist instead of sliding the shoes.

A mesh with no `spine_02` adds that bone's share onto `spine_01`, so the waist still receives it.

The neck then takes back about 55% of the stacked spine yaw. The head keeps its own look-at toward the opponent. That is why the chin stays in the fight while the shoulders turn past it.

Orthodox still plants the hips about 14 degrees to the lead side. Southpaw mirrors that. The coil is added on top.

`ComputeDisplayedPunchYaw` is the punch part of the coil, in degrees, already multiplied by how far the arm has extended (`PunchPoseAlpha`) and by -1 for southpaw. A jab is 6, a straight is 18 the other way, a lead hook is 24, a rear hook is 26, a lead uppercut is 8, a rear uppercut is 10. Those numbers are the coil, not the reach of the punch sphere.

The right stick still turns the capsule, up to `MaxFacingOffsetDegrees` (50). That keeps the punch aimed where the shoulders point. `FacingYawOffset` is replicated so the other machine can lag the hips by the same amount. The owning client writes the smooth value every tick after the server copy arrives, so the local fighter does not stutter.

If the mannequin fails to load, the cylinder body uses the same twist. The trunks follow the hips. The chest cylinder follows the spine. That fallback is there so a missing mesh still shows the coil.

## What a landed punch does to the body

`PlayHurtReaction` runs on the fighter who was hit. It is a drawing. It does not add damage.

| Shot | Waist yaw | Fold (`HurtLean`) | Chin tuck (`HurtHeadPitch`) |
| --- | --- | --- | --- |
| Clean head shot | 16 degrees | 6 | 14 |
| Clean body shot | 10 degrees | 18 | 4 |
| Block | 35% of the matching row | 35% | 35% |

A rear hand (straight, rear hook, rear uppercut) flinches the other way from a lead hand. Southpaw mirrors the sign. `DecayHurtReaction` eases all three values back to zero. The speed is 6, so a clean shot reads for about a third of a second and then the guard returns.

The skull used to scale up on impact. That read as a growing head. The flinch replaced it.

On screen, a landed or blocked shot prints one line, for example `Head  clean  12` or `Body  blocked  2`. The output log on the server also prints the pools after the add: `FinalBell damage region=head amount=12.0 head=12.0 body=0.0 stun=16.2`.

## The three damage numbers

`ApplyAuthoritativeDamage` is the only function that changes them, and it returns immediately unless this machine is the server. It also returns if this punch id was already applied. One swing cannot hurt twice.

| Pool | What it means | What fills it | What it changes |
| --- | --- | --- | --- |
| `HeadDamage` | Lasting chin damage. Cap `MaxHeadDamage` (100). | Head shots, including a blocked head shot at the reduced amount. | The record. It does not permanently weaken later punches. The stun from the same shot does, and then the stun fades. |
| `BodyDamage` | Lasting rib damage. Cap `MaxBodyDamage` (100). | Body shots (hold C, L1, or LB). | Energy refill, a share of outgoing power, and a share of walk speed. |
| `Stun` | The wobble right now. Cap `MaxStun` (100). | Head damage times 1.35, or body damage times 0.65. | Outgoing power and walk speed, until `DecayStun` removes it. |
| `AccumulatedDamage` | Head plus body. | Updated whenever either pool changes. | Older readers that expect one total. |

A blocked punch already arrives as about 15% of the clean amount. That smaller number is what enters the pool and what raises stun. There is no second block cut inside `ApplyAuthoritativeDamage`.

A body shot is also multiplied by 0.85 before it is applied. It deals a bit less and it is the shot that takes the wind. A power shot is multiplied by 1.4. An angle bonus after slipping a hook is 1.5. Those multipliers happen in `ResolveAgainstOpponent` before the pools are touched.

`GetOutgoingDamageScale` is applied to the puncher, on the server, before the defender is hurt. At full body damage it removes 25% (`BodyOutputPenalty`). At full stun it removes 15% (`StunOutputPenalty`). The floor is `MinOutputScale` (0.55), so a hurt fighter can still punch. Lasting head damage is left out of this scale on purpose: the chin is the stun spike, and a high head total should not quietly shrink every punch for the rest of the bout.

`GetBurstRecoveryScale` is used in `TickComponent` when burst energy refills. At 0 body damage the pause bonus is full speed (8 energy per second, plus a trickle that keeps running during a punch). A high guard still refills, at about 70% of that bonus. At full body damage the refill is `BodyRecoveryAtFull` (0.45), so 45% of that speed. Head damage does not change this. The gas tank and the chin are different resources.

`GetMoveSpeedScale` shortens the step. Full stun removes 28% of walk speed. Full body damage removes 12%. The floor is 62% of `BaseWalkSpeed` (320). `ApplyConditionToMovement` writes that into `MaxWalkSpeed` on every machine, from the replicated pools. The capsule stays 34 by 90. Punch reach does not grow or shrink because someone is hurt.

`DecayStun` runs only on the server, at 18 points per second. A jab's stun (about 8) is gone in under a second. A harder shot lasts longer. Clients display the replicated value, so the number can step slightly instead of falling every frame on the client. That is the copy of the server's value, not a second drain.

## Walk through one straight to the head

1. You press 2, or flick the right stick forward. `SubmitLocalIntent` builds an `FFBCombatIntent`. The intent names the punch. It has no damage field.
2. The server accepts it in `ServerTryProcessIntent` if the fighter is idle and can afford the energy. `BeginAttack` spends the energy and starts the startup phase.
3. When startup ends, `ResolveAgainstOpponent` checks reach, facing, the punch sphere, the guard, and slips. A high guard blocks a head shot. It does not block a body shot.
4. The server multiplies base damage (a straight is 12) by power, by the body-shot factor, by the angle bonus, and by `GetOutgoingDamageScale`.
5. `ApplyAuthoritativeDamage` adds that amount to `HeadDamage`, adds stun, and sets `AccumulatedDamage` to the sum.
6. The impact replicates. `HandleImpactPresentation` calls `PlayHurtReaction` on the defender. The waist yaws, the chin tucks, and the line `Head  clean  12` appears.
7. Over the next seconds the server fades stun. Energy refill is unchanged, because this one was a head shot. Walk speed dips while stun is up, then returns.

A body straight does the same walk, with `Region` set to body. The amount goes into `BodyDamage`. The flinch folds the waist. Later punches from that fighter refill energy more slowly and hit a little softer. At the bell, `RestoreForNextRound` keeps about 68% of the chin damage and about 58% of the body damage.

## Seeing the numbers

The corner panels are `SFBFightMeters`. They appear after you leave the mode menu.

| Bar | Color | What the fill means |
| --- | --- | --- |
| STAMINA | Gold | Burst energy left. It drops when you punch and climbs back while you box, slower behind a guard. Fatigue keeps it from reaching the top. |
| HEAD | Danger red | Chin damage, 0 to 100. |
| BODY | Leather brown | Rib damage, 0 to 100. |
| STUN | Pale gold | The wobble. It climbs on a hit and falls on the server. |

The left panel is you. The right panel is the opponent. In training the right name reads BAG and those bars stay empty, because the bag does not take these pools. A line such as `Head  clean  12` still appears on a landed shot.

## The 1v1 AI room

Choosing 1v1 vs AI passes `bOldSchoolGym` into `EnsureArena`. The 20 ft ring, the ropes, and the invisible play box stay. The dark bowl around them is replaced by `SpawnOldSchoolGym`: a wood floor, brick walls, a low ceiling, beams, dirty windows, four heavy bags on the far wall, a bench, a bucket, a mirror, and two posters. The lights go warm, and four bulbs hang under the ceiling. The bags and walls do not block. Multiplayer and training keep the dark bowl.

## Where to change the feel

On the boxer, category **FinalBell | Damage**:

| Property | Default | What turning it does |
| --- | --- | --- |
| `BaseWalkSpeed` | 320 | Speed before any injury. |
| `MaxHeadDamage` | 100 | How much chin damage fits in the pool. |
| `MaxBodyDamage` | 100 | How much rib damage fits in the pool. |
| `MaxStun` | 100 | How much wobble fits before it stops rising. |
| `HeadStunMultiplier` | 1.35 | How hard a head shot rocks balance. |
| `BodyStunMultiplier` | 0.65 | How hard a body shot rocks balance. |
| `StunRecoverPerSecond` | 18 | How fast the wobble fades. |
| `BodyRecoveryAtFull` | 0.45 | Energy refill when the body pool is full. |
| `BodyOutputPenalty` | 0.25 | Power lost at full body damage. |
| `StunOutputPenalty` | 0.15 | Power lost at full stun. |
| `MinOutputScale` | 0.55 | The least power a hurt fighter keeps. |
| `StunMovePenalty` | 0.28 | Step lost at full stun. |
| `BodyMovePenalty` | 0.55 | Step lost at full body damage. A lot of body shots should look slow. |
| `MinMoveScale` | 0.34 | The slowest the step gets. |

Punch base damage and energy cost stay on the combat component, category **FinalBell | Combat | Tuning**.

The waist shares are the four coefficients in `BuildWaistTwist`. Raise the `spine_01` share if the belt still looks quiet. Keep the stick's three spine shares equal to the hip lag (0.75) if the shoulders should stay on the punch sphere.

## What this pass does not do

- It does not stop the fight at 100, and it does not roll a flash knockout.
- It does not swell the face, open a cut, or spawn blood.
- It heals some of the head and body pools at the bell. It does not wipe them.
- It does not move the punch spheres onto the gloves. The spheres stay on the capsule.
- Play in the editor was not run as part of writing this page. A compile result, when one exists, is in `docs/Progress.md`.

Empty burst energy is a separate rule. Spending the gas tank to nothing starts the referee's count. That clock, the stick get-up, and the three-judge card are in `docs/BoutAndScorecard.md`. Filling the head or body pool still does not end the bout.
