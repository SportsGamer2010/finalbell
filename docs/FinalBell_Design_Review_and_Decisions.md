# Final Bell — Design Review and Two-Person Implementation Decisions

17 October 2026 · Working baseline prepared from Brian's request and the supplied partner design

The supplied plan contributes a coherent long-term vision: boxing controls remain primary, fighter development has causes, careers evolve without constant management, and official online state is isolated from local saves. Its main production problem is the amount of interdependent work before two players can prove that a punch looks and feels correct over a network.

The recommended first proof is one ring, two boxers, one division, credible jab/cross/guard, stamina, rounds, a basic AI and a two-client combat test. The larger career and championship vision is retained. The master prompt turns it into ordered gates rather than a request to implement a studio-sized feature list at once.

## What the partner plan adds

| Contribution | Why retain it |
| --- | --- |
| True ability, scouting estimates and readiness are separate | A win changes evidence about a boxer; it does not magically create punch power. |
| Autonomous offline world | Careers can have rivals, prospects, aging, recovery and history without depending on public servers. |
| Championship lineage and eligibility | Titles can have a meaningful history beyond a leaderboard position. |
| Optional promotion/stable layer | Long-term depth remains available without requiring everyone to play as a manager. |
| Distinct player rating, fighter ranking and title eligibility | Competitive matchmaking and a believable boxing career need different decisions. |
| Domain/universe isolation | Local experimentation and future VR cannot corrupt official controller competition. |
| Versioned contracts and idempotent results | Repeated messages or server retries cannot multiply wins, payouts or belts. |

## Corrections incorporated into the Cursor baseline

| Supplied assumption or gap | Implementation decision |
| --- | --- |
| Multiple planning sections describe different phase structures and solo staffing. | Use the master prompt's P0–P6 gates, one accountable owner and one reviewer per task, and two shared contract reviews. Dates remain uncommitted. |
| The first 30-session sequence explicitly postpones multiplayer; a two-player prototype appears late in M16/P5. | Server-aware combat starts immediately and P1 includes a two-client smoke test. Public online operations remain a later gate. |
| Exact replay/determinism language is broader than the implementation proves. | Exact replay is required for pinned headless models. Real-time fights are auditable from authoritative state/events and UE replay; cross-build bit-for-bit physics reconstruction is not assumed. |
| The earlier prompt suggested a TypeScript backend. | Adopt the partner plan's Python/FastAPI/PostgreSQL boundary; keep combat in native Unreal C++. |
| Offline careers depend on an unresolved Python distribution decision. | Run a clean-machine packaging spike before that dependency enters the shipping career loop. Editor Python is not the runtime solution. |
| Event sourcing and infrastructure could overwhelm a two-person prototype. | Begin with finalized results, selected events, snapshots and a modular backend. Add a worker/outbox at the relevant gate; scale services only after measurements. |
| The representative SQL examples use insufficiently scoped independent references. | Treat them as illustrations. Implement composite domain/universe constraints and concurrency tests before official data. |
| Source code is described as previously inspected/verified. | This review did not independently inspect or test FightVantage. Treat every extraction claim as pending repository, commit, license, dependency and test verification. |
| Facial swelling, cuts/blood and exceptional flash KOs are not explicit enough in the partner's delivery cards. | Preserve regional swelling/morph assets, wounds, cosmetics replication, KO eligibility and acceptance scenarios as explicit P2 work. |
| Normalized Ranked and Online Universe could unintentionally share consequences. | Separate their pools/ledgers. Each bout declares which record/rating/title policies it can affect. |
| Tamper-proof progression is implied for locally controlled offline saves. | Protect official servers and prevent imports; do not promise to stop a user editing their own offline data. |
| A disconnect category could be treated as proof of intent. | Use evidence-based grace/forfeit policies. Do not infer voluntary quitting from network failure alone. |

These are implementation defaults for the current merged plan, not evidence that the code has already implemented them. New user decisions supersede this document.

## Name and compatibility policy

| Use | Name |
| --- | --- |
| Player-facing product | Final Bell |
| New Unreal project/native module | FinalBell |
| Repository/product slug | final-bell |
| Python namespace | final_bell |
| Extracted simulation package | final_bell_sim |

For a new project, use these names consistently. For an existing Unreal project, display branding can change before native modules do. Module/class/package renames need appropriate redirects and save/asset migration checks. Keep required compatibility identifiers rather than breaking existing assets to achieve a cosmetic text-search score. FightVantage remains the separate candidate source project's actual name.

## One realistic scope boundary

| Gate | Demonstrable outcome | Keep out of the gate |
| --- | --- | --- |
| P0: foundation | Both people reproduce the build, share contracts and identify missing assets/tools. | Public hosting, VR and full management menus. |
| P1: boxing/network proof | Three-round standalone bout and a two-client authoritative jab/cross/guard test. | Full world economy, broad roster and ranked services. |
| P2: core modes | Training, varied AI sparring, full defense, damage/flash KOs and regional visual damage hooks. | Face scanning, numerous venues and cinematic production. |
| P3: offline career slice | Create/train/fight/save loop, a small evolving world, readable records and one title path. | Every division, every organization and complex promoter finance. |
| P4: competitive online | Dedicated authority, authenticated players, records, one ranked queue and robust result/failure handling. | Splitting a small audience across many simultaneous ladders. |
| P5: living-world depth | Generational careers, more belts/geography, private leagues and measured expansion. | Treating every backlog idea as a release blocker. |
| P6: separate expansion | Proven console/VR feasibility with appropriate budgets and isolated VR competition. | Unproven cross-domain competitive transfers. |

Initial content target: one ring, one division, two usable fighter rigs and three tunable AI style profiles. More fictional names can be data fixtures without requiring a unique finished model for each. Validate normal and power strikes against one consistent skeleton before purchasing or importing a large asset collection.

## Suggested two-person split

The assignment below is a starting arrangement, not a claim about either person's technical skill. Swap ownership if the partner is stronger in Unreal. Each work item still needs one owner.

| Area | Proposed owner | Partner's review responsibility |
| --- | --- | --- |
| Controls, footwork, camera and punch feel | Brian / Developer A | Play the build and check contact/state contract compatibility. |
| In-ring AI execution, HUD and training presentation | Developer A | Check rule legality and that AI uses the same resources as players. |
| Python models, scouting, aging and headless simulation | Friend / Developer B | Compare with playable telemetry and verify player-facing explanations. |
| Career saves, rankings, title rules and online API/data integrity | Developer B | Check UX, exception behavior and gameplay consequences. |
| Network integration and result boundary | A owns UE server; B owns certification/backend | Joint two-client and forged/duplicate result tests. |
| Shared schema, tuning and public build acceptance | Joint review; one named task owner | Both approve the interface and test the packaged build. |

Use a shared repository, short branches, Git LFS for large UE assets, binary-asset ownership/locking where available, and a consistently playable main branch. Avoid simultaneous edits to the same map, animation Blueprint or widget. The collaborator changing a shared contract also supplies a versioned example payload and the migration/fixture change.

Agree in writing on ownership of code/assets, responsibilities, spending approvals, revenue allocation, decision authority and departure arrangements before major spending or release. This is a collaboration planning recommendation, not a drafted partnership agreement.

## First bounded tasks

| ID | Owner | Work | Evidence |
| --- | --- | --- | --- |
| FB-001 | A | Audit engine/compiler/assets, establish product naming and open the actual project. | Exact toolchain and a clean build/run log. |
| FB-002 | B | Draft FighterTrueState, FighterReadiness, CombatIntent and FightResult fixtures. | Schema validation with version/domain rejection cases. |
| FB-003 | A | Ring bounds, two placeholders, controller input and camera. | Playable movement demonstration. |
| FB-004 | B | Pure impact/resource reference cases and expected legality outcomes. | Small documented test corpus, no database dependency. |
| FB-005 | A | Authoritative jab/cross/guard and resource state. | Hits, misses and guard interception match debug traces. |
| FB-006 | A, B reviews | Two separate clients exercise the same combat path. | Consistent legal contact/resource changes and measured connection conditions. |
| FB-007 | A | Three-round flow, valid result and a basic legal AI. | A complete bout without soft-lock or duplicate finalization. |
| FB-008 | B | Result repository/idempotency fixtures; no public service yet. | Duplicate results apply once; inconsistent participants reject. |
| FB-009 | A | One fighter damage-art integration spike. | Left/right swelling/cut state preview on a suitable real mesh, or an explicit missing-asset blocker. |
| FB-010 | B | Offline worker/runtime packaging experiment. | Clean PC without Python/internet can advance a bounded world action, or a documented failure/alternative decision. |

These tasks overlap only where their contracts and files are independent. They do not authorize multiple agents to write the same project or imply that Cursor can access the friend's machine.

## Six required enhancements and remaining suggestions

Revision 2.1 promotes the six enhancements listed in Brian's follow-up into explicit phased implementation requirements. Their identifiers and acceptance criteria are in the master prompt. They are planned features, not claims of implemented gameplay. Preserve a small prototype while tracking every later requirement.

| ID | Required feature | Delivery gate |
| --- | --- | --- |
| FB-ENH-01 | Signature defense with rewarding slips, angles, positioning and counters | P2 |
| FB-ENH-02 | Flash-KO replay explaining the verified strike/opening | P2 telemetry, P3 viewer, P4 online parity |
| FB-ENH-03 | Specific actionable training/bout feedback from telemetry | P2 |
| FB-ENH-04 | Dedicated scouting cards with evidence and uncertainty | P3 |
| FB-ENH-05 | Linked opponents/cards/title histories and a rank-change explanation UI | P3 |
| FB-ENH-06 | Private rooms, one initial ranked pool, scheduled fight nights and basic private leagues | P1 rooms, P4 ranked, P5 events/leagues |

The list below retains the design rationale. Entries 1, 3, 4, 5, 6 and 7 describe these required enhancements. Additional ideas, including advanced league/API integrations, broad replay editing and world storytelling, remain optional or separately gated. A later milestone does not make one of the six required features optional.

1. **Make defense enjoyable and readable.** Reward a clean slip with position and counter opportunity, show why a punch missed, and make cutting off the ring effective. Validate that a defensive boxer can win without constant retreat or excessive punch volume.
2. **Make fighter style visible in play.** A pressure fighter should close space and target the body; an out-boxer should manage the jab/range; a counterpuncher should create punishable openings. Begin with decision/animation/pacing differences rather than dozens of opaque stat modifiers.
3. **Turn training into useful feedback.** After a drill/bout, show three actionable observations derived from telemetry: low-stamina power punches, repeated exposure to the same counter, or attacks thrown outside range. Start with deterministic templates; a cloud LLM is unnecessary for the core feature.
4. **Give exceptional outcomes context.** A flash-KO replay should identify the clean strike, counter window and fatigue/guard context. An unexplained random defeat feels less fair than an exceptional but understandable finish. This is a presentation goal to playtest, not a guarantee that every loss will feel fair.
5. **Create an opponent scouting card.** Show recent opponent habits, estimated strengths and uncertainty. Give film study a practical purpose without revealing a hidden perfect answer to every style.
6. **Make BoxRec-style records interactive.** Link every opponent, title reign and card; show “why this rank changed” and division/history filters. Keep numerical fighter ability distinct from public ranking and matchmaking skill.
7. **Start online with private rooms and one clear ranked pool.** Scheduled fight nights and direct challenges can support meaningful matches without immediately fragmenting play across many divisions, modes and regions. Expand queues using actual player availability and measured connection quality.
8. **Offer adjustable career administration.** “My Boxer” provides the direct experience; optional stable/promotion tools add depth. Training delegation should remain viable so a player can spend time fighting.
9. **Make corner choices understandable.** Later between-round choices can prioritize tactical advice, breathing/recovery or cut treatment under a bounded ruleset. Present the tradeoffs clearly and avoid instant full recovery.
10. **Treat leagues as a future differentiator.** Private organizations, season schedules, brackets and a read-only certified-results API could support Brian's existing league/community work. Keep this downstream of proven 1v1 play; do not connect or modify existing websites automatically.
11. **Plan replay/clipping from authoritative markers.** A small event taxonomy for knockdowns, clean counters and stoppages can later support spectator overlays, coaching review and highlight exports. Do not build a video-production platform during the combat prototype.
12. **Use event-driven world stories.** Rivalries, mandatory challenges, retirements and prospect breakthroughs should follow actual world events. Begin with local templates so offline play remains complete.

## Important unresolved evidence

- The actual game repository, partner's development environment and playable build were not attached. This task prepares implementation material; it does not rename or compile an unseen game project.
- The linked FightVantage repository could not be retrieved during this review. No module correctness, license, production fitness or calibration claim has been independently verified.
- No final fighter meshes, swelling targets, authored punch animations, reference hardware or online hosting budget were supplied. They remain explicit implementation inputs rather than assumed completed assets.
- Exact controls, online calendar speed, stat-normalization policy and ranking weights remain tunable product decisions with prototype gates. Neither the old planning durations nor the number of people establishes a reliable release date.

## Technical references

- [Epic: Networking Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine) — supports planning networking early.
- [Epic: Scripting the Unreal Editor Using Python](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python) — editor Python is not a packaged gameplay runtime.
- [Epic: Using the Replay System](https://dev.epicgames.com/documentation/unreal-engine/using-the-replay-system-in-unreal-engine) — reconstructs gameplay from recorded replicated data.
- [Epic: FBX Morph Target Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-morph-target-pipeline-in-unreal-engine) — face deformation requires compatible authored mesh data.
- [Cursor: Rules](https://cursor.com/docs/rules) — project rules use `.mdc` files in `.cursor/rules` with frontmatter.

The game-design suggestions above are this review's recommendations. The technical references do not independently validate the proposed game balance or commercial prospects.
