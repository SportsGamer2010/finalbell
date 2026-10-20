# Boxing AI

The opponent in sparring and career is `AFBBoxerAIController`. It runs a native state machine and a small utility scorer. Punches still go through `UFBCombatComponent::ServerTryProcessIntent`. The AI does not write damage, stamina, health, or the winner.

Multiplayer stays two humans. Heavy-bag training does not spawn a fighting opponent.

## Difficulty

On the AI fighter screen, the bottom bar has **AI: NOVICE**, **AI: CLUB**, or **AI: EXPERT**. Club is the default.

The red bar moves with the left stick or the D-pad. Cross (or Enter) cycles the highlighted row. Circle (or Escape) backs out. The choice is saved in the existing player-settings slot, so an older save still loads and comes up as Club. The server stores the value with the pending bout. Rematch keeps that same value. Career uses the same row.

Difficulty changes recognition delay, decision spacing, how often a lower-scored punch is picked, how long habits are remembered, and how long an exchange may run. It does not change ratings, damage, reach, walk speed, or stamina.

| | Novice | Club | Expert |
| --- | --- | --- | --- |
| Decision spacing | 0.20s | 0.15s | 0.10s |
| Recognition | 0.16s | 0.07s | 0.035s |
| Exchange cap | 1 punch | 2 | 3 |
| Missed reads | common | some | rare, not zero |

A jab's startup is 0.08s for every fighter. Novice recognition is longer than that, so a novice often shells up from range before the punch rather than slipping the startup. Expert can be on time and can still be wrong.

## States

`Measure`, `Approach`, `Exchange`, `Defend`, `Recover`, `EscapeRopes`, `Hurt`, `Inactive`.

A knockdown, a pause, a clinch, a missing opponent, or a phase that is not the live round wins immediately. Other changes wait out a minimum dwell (default 0.40s) unless the gas tank, the ropes, or a seen punch forces the exit. Recover starts under 22% of the fatigue ceiling and leaves near 46%, or at the highest energy the ceiling can still reach when fatigue has crushed the tank. One small regen tick does not clear it. At a safe distance the AI sends a no-attack intent that drops the guard, because energy only returns while the guard is open. A punch intent does not drop the guard.

Footwork keeps stick strength. A short desired step stays a short `AddMovementInput`. Walk speed is not raised.

## Tuning

Defaults live on `AFBBoxerAIController` as `Tuning` (`FFBTacticalTuning`). Change them in C++. There is no binary asset.

Style shape lives in `FFBStylePolicy::ForStyle`. Out-boxers and jab technicians keep range and short exchanges. Swarmers, in-fighters, and body punchers enter covered and work the body. Counterpunchers and slick boxers draw and answer. Sluggers and power punchers pick fewer heavy shots. High guard and peek-a-boo enter covered and still have to drop the guard to refill. Stance is separate: a southpaw counterpuncher stays a counterpuncher and still steps to the lead side.

Punch legality uses reach, facing, the glove band, and predicted movement. A style's preferred distance does not make a punch connect.

## fb.DebugAI

In the console:

```
fb.DebugAI 1
```

The output log, and a silver engine debug line, show state, style, difficulty, why the state changed, energy, fatigue ceiling, guard, enter and leave lines, distance, preferred range, movement bucket, stick strength, the top scores, the chosen action, the last intent result, and cue age. `fb.DebugAI 0` turns it off. It is not drawn on the bout HUD.

The same object counts exchange length, entry speed, dwell, rejects, misses, rope stalls, recoveries, and response latency. Those counters reset when the opponent changes, on a knockdown, on a mode change, on rematch, and when the controller is destroyed.
