# Final Bell — Architecture

Last updated: 17 October 2026 · Gate: P0 / early P1

## Product boundaries

| Layer | Owner tech | Responsibility |
| --- | --- | --- |
| Real-time combat | Unreal C++ (`apps/game-unreal`) | Movement, contact, resources, referee clock, replication, presentation hooks |
| Presentation / tuning | Blueprint + data assets | Designer-facing parameters; no giant authoritative graphs |
| World / career / rankings | Python (`packages/domain-python`, later workers/API) | Pure simulation, contracts, persistence workers — never combat frame loop |
| Shared contracts | `packages/contracts` | Versioned JSON schemas + fixtures consumed by C++ and Python |

## Runtime authority

- Server owns attack legality, hit confirmation, stamina/fatigue, impact IDs, bout phase, and result finalization.
- Clients send `CombatIntent` (sequence + time context). Clients never send authoritative damage/wins.
- Listen-server is allowed for unranked P1 proofs. Dedicated server target exists (`FinalBellServer.Target.cs`); official ranked packaging is a P4 gate.
- Domains: `TRADITIONAL` / `VR`. Universes: offline local, private league, normalized ranked, online universe — separate ledgers.

## P1 combat slice (implemented as C++ stubs)

- `AFBBoxerCharacter` — stance, eight-way move input hooks, intent submission RPC
- `UFBCombatComponent` — burst energy, fatigue ceiling, jab/cross/guard, reach check, guard intercept, attack-instance dedupe
- `AFBSparringGameMode` — 3-round phase machine with one-shot finalize
- `AFBBroadcastCamera` — frames both fighters
- `AFBRingBounds` — box clamp helper
- `UFBPrivateRoomSubsystem` — room code create/join foundation + transport notes (FB-ENH-06 P1)

## Deferred (tracked, not in this increment)

Full punch/defense set, swept socket contact vs authored motion, GAS attributes init, facial regional damage presentation, training modes, AI styles, offline career, ranked EOS/Steam, Python offline worker packaging spike.

## FightVantage

Separate candidate repository. Not present in this workspace. No code copied. Provenance required before any extraction.
