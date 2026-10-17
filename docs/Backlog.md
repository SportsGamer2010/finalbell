# Final Bell — Backlog

Owners: **A** = Brian (UE/combat), **B** = partner (contracts/world). One owner + one reviewer per task.

## Active milestone (P0 → P1)

| ID | Owner | Reviewer | Prereq | Files | Acceptance | Evidence | Next |
| --- | --- | --- | --- | --- | --- | --- | --- |
| FB-001 | A | B | — | Audit docs | Toolchain recorded | Progress 2026-10-17 | Done (green editor module) |
| FB-001b | A | B | Green build | `Content/Maps/Ring_P1`, IMC | PIE movement + camera frames two fighters | Pending editor assets | FB-003 / FB-005 playtest |
| FB-002 | B | A | Python | `packages/contracts/**` | Schema reject cases | Schemas committed; pytest pending Python | FB-002b |
| FB-002b | B | A | Python | fixtures + Elo module | pytest green | Pending | FB-004 impact cases |
| FB-003 | A | B | FB-001b | Ring map, camera | Playable movement demo | Pending | FB-005 |
| FB-004 | B | A | FB-002 | `tests/golden` impact legality | Documented corpus | Pending | Support FB-005 |
| FB-005 | A | B | FB-003/004 | Combat component | Hit/miss/guard match traces | Stub present; needs playtest | FB-006 |
| FB-006 | A | B | FB-005 | Net smoke script/docs | Two clients, measured RTT/loss | Pending | FB-007 |
| FB-007 | A | B | FB-005 | GameMode AI | Complete 3-round bout | Phase stub only | FB-008 |
| FB-008 | B | A | FightResult | Result repo fixtures | Idempotent finalize | Schema only | P3 saves |
| FB-ENH-06a | A | B | FB-001b | `FBPrivateRoomSubsystem` | Create/join private bout | Code stub; needs session test | P4 ranked |

## Required enhancements (see FeatureStatus.md)

| ID | Gate | Owner | Status |
| --- | --- | --- | --- |
| FB-ENH-01 | P2 | A | Planned |
| FB-ENH-02 | P2/P3/P4 | A | Planned |
| FB-ENH-03 | P2 | A (+B fixtures) | Planned |
| FB-ENH-04 | P3 | B | Planned |
| FB-ENH-05 | P3 | B | Planned |
| FB-ENH-06 | P1/P4/P5 | A/B | P1 foundation in progress (not verified) |

## Deferred vision (retain, do not pull into P1)

Quick Fight exhibitions beyond sparring stub · My Boxer career · Offline sandbox worldgen · Online Universe · Private leagues/fight nights (P5) · Generational prospects · Promotion economy · Console/VR (P6) · Face scanning · Kafka/service mesh · Broad admin apps
