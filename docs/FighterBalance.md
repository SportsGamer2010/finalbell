# CompetitiveFair fighter builds

The ruleset id is `CompetitiveFair`, version 1. There is one cost function, in `FFBFighterBalance`. The creation screen, the save, the match setup, and `Tools/AI/scripts/check_fighter_balance.py` all use that same curve. An earlier 1100-point guess is not used.

These numbers are a first tuning pass. Matching the point total does not mean the game is balanced. That still needs play.

## What a legal fighter is

Nineteen integer ratings, each from 50 to 95. A draft may cost less than 600. A competitive bout accepts a build only when the cost is exactly 600. Unknown ratings, values outside 50–95, and a cost over 600 are refused. They are not quietly clamped.

Cost is counted up from 50:

| Rating | Cost |
| --- | --- |
| 50 | 0 |
| 51–70 | 1 each |
| 71–80 | 2 each |
| 81–90 | 3 each |
| 91–95 | 5 each |

So 60 costs 10, 70 costs 20, 75 costs 30, 80 costs 40, 85 costs 55, 90 costs 70, and 95 costs 95. Nineteen ratings at 80 would cost 760, which is illegal.

Career XP, the win-loss record, trophies, and account age do not add points or a damage multiplier.

## What each rating does

`N` is `(rating - 50) / 45`, clamped from 0 to 1. At 50 every multiplier below is 1, so an unselected fighter feels like the current boxer.

| Rating | Effect |
| --- | --- |
| Chin Resistance | Incoming head damage is `1 - 0.12N`, applied once. The head-daze line is `70 * (1 + 0.05N)`. |
| Body Resistance | Same for body damage and the body-daze line. |
| Cut Resistance | Cuts open and worsen at `1 - 0.20N`. Never zero. An open cut adds at most 4% incoming head damage. |
| Bruise Resistance | Bruises worsen at `1 - 0.20N`. A bruise adds a small extra stun, capped. |
| Guarding | A correct high guard loses block at `1 - 0.10N`. It does not track punches. |
| Courage | The get-up window is 5% wider and the hold is 5% shorter at 95. It does not add a get-up. |
| Block Health | Guard pool is `100 * (1 + 0.15N)`. |
| Block Regen | Guard returns only after 0.8s with no blocked shot, at `1 + 0.15N`. |
| Reflexes | Weave step is up to 10% longer. A miss recovery is up to 8% shorter, and never under 75% of the normal recovery. |
| Footwork | Walk speed is up to 10% faster. The capsule does not change. |
| Punch Speed | The punch finishes up to 10% sooner. It still commits. |
| Accuracy | The legal stick cone is `25 + 3N` degrees before the punch starts. Rating 70 is about 26 degrees. A committed punch is not an accuracy roll. |
| Stamina | The gas tank is `100 * (1 + 0.20N)`. |
| Stamina Regen | Refill is `1 + 0.15N`, and only after 0.6s without a punch, guard down, standing idle. |
| Stamina Recovery | Between rounds, 55% of the missing gas comes back, times `1 + 0.15N`, and never more than the deficit. |
| Conditioning | Miss fatigue is `1 - 0.05N`. It is not a second regen stat. |
| Endurance | Punch cost is `1 - 0.15N`, and the cost stays at least 0.5. |
| Left / Right Hand Power | That physical hand's damage is `1 + 0.20N`. Orthodox lead is the left hand. Southpaw lead is the right hand. Switching stance does not swap the ratings. |

Power, body-target, and angle used to multiply without a ceiling. A power shot (1.4) with an angle bonus (1.5) was 2.1. Anything over 1 is now capped at 1.60. A miss is 0. The same attack id cannot apply damage twice.

Final damage is base punch damage, times hand power, times the capped exposure, times the existing outgoing energy scale, times chin or body resistance once.

A daze that crosses the head or body line uses the existing knockdown. The shock resets, so a durable fighter can be dropped again. Courage does not erase the extra stun after the count.

## The eight fighters

Division is a label. Pairing uses lightweight, middleweight, and heavyweight.

| Name | Division | Class | Style |
| --- | --- | --- | --- |
| Marcus Hale | Heavyweight | Heavyweight | Slugger |
| Ivan Petrov | Cruiserweight | Heavyweight | In-fighter |
| Leon Voss | Light heavyweight | Middleweight | Counterpuncher |
| Samir Quinn | Middleweight | Middleweight | Boxer-puncher |
| Diego Marte | Welterweight | Middleweight | Swarmer |
| Ellis Kane | Lightweight | Lightweight | Out-boxer |
| Nico Abebe | Featherweight | Lightweight | High guard |
| Rafi Solano | Bantamweight | Lightweight | Southpaw |

Each premade is its own 600-point build. The Balanced, Puncher, Counter, and Pressure presets remain as creator shortcuts. They are not the roster. Bantam, feather, and lightweight share a class. Welter, middle, and light heavy share a class. Cruiser and heavy share a class. Different classes are refused. Voss is a southpaw counterpuncher. Solano is the southpaw stylist. Marte's swarmer is separate from Petrov's in-fighter.

## Creating and saving

1v1 AI opens the fighter screen instead of starting immediately. `FBMode 1` does the same. Pick a premade, or create one. Created fighters save in the slot `FBFighterRoster`, up to 10. Premades do not count. Before the first fight you can spend any legal draft. After a completed fight, that camp can refund up to 20 points, and the saved result must still cost 600. The server copies the build onto the fighter when the match is placed and will not apply an illegal one.

Race (complexion), hair, brow, jaw, nose, tattoos, and muscle are presentation. They tint the body, add a no-collision hair or tattoo shape, move the face marks a few centimeters, or scale the glove mesh. They do not scale the capsule or the punch spheres.

## What this bout does not do yet

The live bout is still one 3:00 round, so the between-round stamina recovery does not run unless a rest phase exists. The function is there for that rest.

This is a listen server. The client sends the proposed build and the intents. It does not send damage. That is not a dedicated-server anti-cheat.

MetaHuman is installed with the engine and left disabled. Faces in the bout are the mannequin plus the presentation above. Turning on MetaHuman Character is a manual editor step, and those meshes are not imported over the live mannequin.

## Where the code lives

| Piece | File |
| --- | --- |
| Cost, curves, presets, the eight fighters | `Combat/FBFighterBalance.h` |
| Save, max 10 | `Combat/FBFighterSave.h` |
| The 1v1 screen | `Combat/FBFighterSelect.cpp` |
| Applying the build, block, cuts, shock | `Combat/FBBoxerCharacter.cpp` |
| Damage, stamina delay, exposure cap | `Combat/FBCombatComponent.cpp` |
| Same-class apply, career, the card line | `Combat/FBSparringGameMode.cpp` |
| The check | `Tools/AI/scripts/check_fighter_balance.py` |
