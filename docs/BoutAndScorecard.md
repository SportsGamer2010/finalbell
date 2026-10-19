# The 3:00 bout, the count, and the card

This page is the reading version of the knockdown, the clock, and the judges. The function list is in `docs/CodeGuide.md`. The decision is D-014.

The server owns all of it. A client can show the clock and send where the sticks are. A client cannot decide that you got up, and it cannot write the scorecard.

## One three-minute bout

`RoundDurationSeconds` is 180. `TotalRounds` is 1. When that clock reaches zero and nobody has failed the count, `BuildDecisionCard` runs and the bout is over.

Training does not run this clock and does not start knockdowns. The heavy bag stays a heavy bag.

The clock pauses while the referee is counting. A knockdown in the last seconds still gets the full ten.

`AFBSparringGameState` carries the clock, the count, the two target angles, who is down, and the card. The game mode does not replicate, so the other screen would never see those numbers if they stayed on the mode.

## Getting knocked down

A punch spends burst energy. When that spend lands on empty, `BeginAttack` cancels the punch and tells the mode. The punch does not get to land from the canvas.

`BeginKnockdown` then:

- marks that fighter down
- starts the count at 10
- remembers the knockdown on that corner's ledger
- stands a referee in front of them
- cuts the camera to the canvas, looking up at the referee

The mesh tips over. The capsule does not change size, and the punch spheres stay where they were. Appearance does not rewrite the competitive shape.

While you are down you cannot walk and you cannot punch. The standing fighter cannot punch you either. The server refuses both.

The first time the AI is dropped, it gets up on its own after about 3.2 seconds. The second time, it stays down. The stick check is for the human. A second empty tank can drop a human who already got up.

Beating the count puts about 46 energy back, so you are not dropped again on the next step.

## The two circles

The server turns the marks. The left mark spins one way at 78 degrees a second. The right mark spins the other way at 112 degrees a second. Your machine draws them. It does not decide that they matched.

Both sticks have to be pushed out (about 58% of the way) and within about 24 degrees of those marks, and held there for about a third of a second. That hold is scored on the server, so a late packet does not grant the get-up by itself.

On a pad, the left stick is the left circle and the right stick is the right circle. On a keyboard, WASD is the left circle and the arrow keys are the right circle. The arrow keys also angle the shoulders during the fight, because they are the same stick the shoulders already read.

## Three judges

Judges vote only when the bell rings and both fighters are still in the bout. A knockdown they beat is not a knockout. It stays on the card as one of the three pillars. A count that reaches ten is still the knockout card, and that card does not invent round scores.

`NoteImpact` still writes the punch-count pillar. A clean punch adds 1. A blocked punch adds 0.25. Head damage and body damage are still recorded on the attacker. The judges do not score those two pools.

`TickRingGeneralship` writes the ring-generalship pillar while the 3:00 is running. It is not a punch total. Each second, a fighter scores for three things:

- They are at the distance their own style wants.
- The other fighter is not at the distance that other style wants, so this fighter made them fight the wrong fight.
- They are closer to the center of the ring than the other fighter.

Knockdowns are counted when the count starts, on the fighter who went down.

The three judges are HARRIS, COLE, and REED. They read the same three pillars and they do not use the same weights:

- Harris cares most about punch count.
- Cole cares most about ring generalship.
- Reed cares most about knockdowns.

A margin under about 1.6 points is a 10-10 round. Otherwise the winner of that card gets 10. The loser gets 9, or 8 if they were dropped or the margin is wide, or 7 if they were dropped twice.

Three cards the same way is unanimous. Two to one is a split. Two cards and a draw on the third is a majority. Tied totals, or no majority, is a draw.

The scorecard lists GENERALSHIP, KNOCKDOWNS, and PUNCHES for blue and red above the three judge rows. Punch credit keeps the clean and blocked split, so a punch line can end in a fraction.

A knockout skips those rows. The card says KNOCKOUT and names the corner still standing.

## What each style is trying to do

The opponent's choices are `AFBBoxerAIController`. The style name picks a small policy: the range they want, which punches they look for, and whether they lead. Feet, power, guard, accuracy, and stamina still come from the 600-point ratings. The policy does not add a second budget, and the intent is the same `FFBCombatIntent` a player sends. Training and the heavy bag do not run this controller.

The full line for each premade style is in `docs/CodeGuide.md`. Generalship uses that same range, including for the human, so the card can see who made the other fighter box at the wrong distance.

## What you see

The overlay is Slate, in the same black, gold, cream, blue, and red as the meters. Oswald is named in the design system and is not imported, so the engine font stands in.

- Top center: a black pill, a gold bar, and the clock, labeled 3:00 BOUT.
- During the count: the view dims, the count is large, and the two circles sit under GET UP.
- At the end: a centered official card. A decision lists generalship, knockdowns, and punches, then the three judges and the totals. A knockout names the winner.

This is the layout used by broadcast boxing games such as Undisputed and Fight Night: clock over the ring, a canvas shot for the count, and a card that looks like a commission sheet. The referee is basic shapes. There is no filmed cutscene asset.

## What this pass does not do

- It does not roll a flash knockout. Empty energy is the knockdown you asked for. Head damage still does not end the bout by itself.
- It does not add swelling, cuts, or blood.
- It does not change the ring size.
- Play in the editor was not run as part of writing this page. The compile result is in `docs/Progress.md`.
