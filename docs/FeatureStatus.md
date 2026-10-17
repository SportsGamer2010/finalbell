# Final Bell — Required Enhancement Status

Revision 2.1 · Updated 17 October 2026 (repo bootstrap)

All six features below are explicit implementation requirements. Update status only with real project changes and acceptance evidence. **Planned** means required-but-not-built; documentation alone is never **Verified**.

| ID | Required feature | Gate | Proposed owner | Status | Acceptance evidence | Remaining work |
| --- | --- | --- | --- | --- | --- | --- |
| FB-ENH-01 | Rewarding slips, angles, positioning and counters | P2 | A; B reviews rules | Planned | — | Defense moves, counter windows, scenario fixtures, playtest |
| FB-ENH-02 | Flash-KO replay identifying verified clean strike/opening/context | P2 markers; P3 viewer; P4 online parity | A; B reviews telemetry | Planned | — | Impact IDs → markers → post-bout viewer; no live pause |
| FB-ENH-03 | Specific training/bout feedback with evidence and correction | P2 | A; B owns telemetry fixtures | Planned | — | Telemetry rules, sample-size gates, coaching UI |
| FB-ENH-04 | Scouting card with tendencies, estimates and uncertainty | P3 | B; A owns UI | Planned | — | Observed estimates, confidence, as-of filtering |
| FB-ENH-05 | Linked opponents/cards/title histories and ranking-change explanation | P3 | B; A owns UI | Planned | — | Navigable records + “why rank changed” view |
| FB-ENH-06 | Private rooms, one initial ranked pool, later fight nights and basic league tools | P1 rooms; P4 ranked; P5 events/leagues | A sessions; B league/results | In progress (P1 foundation only) | `UFBPrivateRoomSubsystem` create/join stubs + transport notes in code; **not** a verified private bout yet | Session OSS wiring, two-client join test; ranked pool P4; leagues P5 |

Do not mark Verified until acceptance criteria in the master prompt are demonstrated.
