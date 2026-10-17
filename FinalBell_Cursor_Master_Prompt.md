# Final Bell — Cursor Master Prompt

Merged working specification, version 2.1 · 17 October 2026

Revision 2.1 makes the six listed enhancements explicit implementation requirements with acceptance criteria. Their planned status does not mean they have been coded or tested in a game project.

This prompt combines Brian's requested Unreal Engine boxing game with the partner's complete design. The companion review resolves implementation conflicts; the renamed reference preserves the larger vision. These are requirements for Cursor to implement and verify, not a claim that game code or referenced third-party modules have been tested.

Paste the following into Cursor after opening the existing Unreal project or the intended new project folder. Keep these companion files in the same repository when available.

```text
Act as a senior Unreal Engine 5 C++ engineer, network engineer, boxing gameplay designer, and technical producer for a TWO-PERSON team. Build FINAL BELL through small, verified increments. Change real files; do not stop at a design proposal.

PRODUCT IDENTITY AND SOURCE OF TRUTH
- Display name: Final Bell. Native project/module name for a new project: FinalBell. Repository slug: final-bell. Python package prefix: final_bell.
- Combine this master prompt with docs/FinalBell_Design_Review_and_Decisions.md and docs/FinalBell_Design_Reference.txt. Latest user instructions take precedence; the review's explicit implementation decisions resolve contradictions in the retained reference. If companion files are absent, this master prompt is sufficient to start.
- Preserve the supplied long-term vision, but implement only the active milestone. Deferred requirements remain visible in the backlog.
- Existing-project rename: inspect current branding, .uproject, modules, targets, namespaces, asset references, package IDs, launch scripts, documentation, and save identifiers. Change display branding immediately. If native symbols/modules need renaming, plan redirects and asset/save compatibility, migrate in a small verified change, and compile/package before claiming completion. Do not perform blind replacements in binary assets. Keep necessary legacy compatibility identifiers documented.
- FightVantage is a separate potential source repository; preserve that name and its real URLs. Renaming this game does not rename that independent project.

FIRST WORK: AUDIT THE REAL WORKSPACE
Inspect repository instructions, existing code, engine version/build type, compiler/SDK, plugins, assets, local databases, and current tests. Do not upgrade Unreal or rewrite functioning systems automatically. Use installed headers/source and official version-matching documentation rather than invented APIs.

If no Unreal project exists, create a supported C++ foundation when tools permit. Otherwise give exact missing-tool/editor steps and continue independent text-based implementation. Never create fake .uasset/.umap files or imply an editor action ran when it did not.

Create/update docs/Architecture.md, docs/Progress.md, docs/AssetChecklist.md, docs/ContractRegistry.md, docs/DecisionLog.md, and docs/Backlog.md. Each task has one owner, one reviewer, prerequisites, concrete files, acceptance criteria, actual test evidence, and the next action.

PRODUCT VISION AND MODE SCOPE
Final Bell is a gameplay-first boxing simulation with tactical movement, defense, realistic-style stamina, consequential damage, and a living career world. Promotion management is optional supporting gameplay. Start with Windows PC and traditional controller/keyboard input. Evaluate console feasibility separately; VR is a future independent product/domain.

Initial playable modes:
1. Training: movement, heavy bag, target/mitt and defensive drills, telemetry, recovery practice, reset controls.
2. AI Sparring: configurable rounds, opponent style/difficulty, intensity, and optional damage/KO consequences.
3. Multiplayer: private/unranked 1v1 initially, authenticated ranked play later.

Retain these later capabilities:
- Quick Fight/exhibitions.
- My Boxer offline career, with an optional stable view.
- Offline sandbox with world generation and editable rules.
- Official Online Universe with persistent careers, contracts, rankings and titles.
- Private leagues, events, qualifiers, eliminators and tournaments.
- Generational prospects, aging, retirement, rivalries and championship histories.

A new player must reach a tutorial/exhibition without setting up a promotion. Career and management depth must not obstruct learning the boxing controls. Complete offline career play must work without account login or online services.

TWO-PERSON WORKFLOW
Proposed ownership, adjustable to actual strengths:
- Developer A / Brian: product decisions, Unreal controls/combat, animation integration, camera, HUD, training presentation, and combat playtest acceptance.
- Developer B / partner: versioned data contracts, Python simulation/reference math, career/world logic, records/rankings/title rules, persistence/backend and fixtures.
- Both review interface changes and test each other's packaged builds. One named owner integrates each shared map/widget/animation Blueprint and the .uproject configuration.

Use one monorepo with short-lived feature branches, small reviewed merges, and a playable main branch. Configure Git LFS for appropriate Unreal binaries and locking where supported; check tracking before committing assets. Exclude generated engine output/caches. Keep JSON schemas and numeric tuning files easy to review. Do not overwrite an active collaborator's changes or edit the same binary asset concurrently. Agree contracts before parallel implementation.

SIX REQUIRED ENHANCEMENTS — TRACK EXPLICITLY
These six items are required Final Bell features, not optional suggestions. They remain phased; later delivery does not remove them from scope. Create docs/FeatureStatus.md with one row per ID, owner, milestone, planned/in-progress/implemented/verified/blocked status, acceptance evidence and remaining work. Never mark a feature verified merely because its requirement exists in this prompt.

FB-ENH-01 — Signature defense (P2)
Implement rewarding slips, angles, body positioning, guard and clean counters. A successful evasion should create a plausible positional/counter opportunity under the same stamina/contact rules, without automatic invulnerability or a guaranteed damage bonus. Validate that clean defensive boxing can win under the judges' scoring rules, rather than requiring excessive punch volume. Acceptance: scripted slip/counter, angle-escape and ring-cutting scenarios agree with contact traces, plus recorded playtest feedback on readability and defensive viability.

FB-ENH-02 — Explain exceptional finishes (P2 telemetry; P3 player-facing replay; P4 online parity)
Provide a bounded pre/post-impact flash-KO replay identifying the decisive clean strike and any verified counter opening, guard exposure or stamina contribution. Use authoritative impact IDs, state/event records and compatible replay data. Explain only factors actually present; never invent a counter or fatigue cause. The viewer must show the finish and its relevant context, not just a final still image. Replays must not pause, slow or alter a live authoritative multiplayer bout; offer them after stoppage/post-bout. Acceptance: an eligible clean-counter KO reconstructs with matching impact evidence; an ordinary finish does not receive false counter annotations; online and offline results remain unchanged by replay controls.

FB-ENH-03 — Actionable training feedback (P2)
After drills and bouts, show up to three specific, evidence-based coaching observations: power punches thrown below the configured energy threshold, repeated exposure to the same counter, or attacks outside effective range. Include counts/timestamps where available and one practical correction. Use deterministic telemetry rules initially; respect minimum sample sizes and say when evidence is insufficient. Acceptance: seeded fixtures produce correct advice and counts, and clean fixtures do not invent mistakes. Advice must be understandable to a new player and must not grant unearned permanent attributes.

FB-ENH-04 — Scouting cards with uncertainty (P3)
Build a dedicated opponent card showing observed tendencies, estimated strengths/weaknesses, confidence or estimated ranges, evidence quantity and last-updated/as-of context. Unknown prospects show uncertainty instead of fabricated precision. Film study can improve preparation/familiarity within the existing progression rules. Do not expose hidden TrueAbility as if it were an observed fact or use future bout data. Acceptance: an opponent with little history has appropriately limited estimates; new eligible evidence updates the card reproducibly; historical views exclude later evidence.

FB-ENH-05 — Explorable records (P3)
Make opponent names, bout entries, judge scorecards and championship reigns navigable. Add a player-facing "Why this ranking changed" view showing the previous/current snapshot, relevant eligible bouts, activity/eligibility changes and rules version. Explain computed reasons without inventing a causal story. Acceptance: a sample fighter can be followed through opponents, a scored bout, title lineage and a rank-change explanation; empty/missing history is handled gracefully and controller navigation works.

FB-ENH-06 — Private rooms, one initial ranked pool and later league growth (P1 private-room foundation; P4 ranked; P5 fight nights/leagues)
Provide private challenge/room creation and joining early, with available transport/session support documented. Launch ranked with one initial eligible division/ruleset pool; preserve necessary latency/eligibility limits rather than treating one pool as permission for unfair matches. Add scheduled fight nights and basic private-league creation, membership, fixtures and standings at P5. Queue expansion is a measured later decision; the P5 league tools remain tracked requirements. Acceptance: authorized players can create/join a private bout; eligible ranked matches use the initial pool; a scheduled league fixture records one certified result and updates its standings once, with no duplicate global rewards or cross-universe writes.

TECHNICAL BOUNDARIES
- Unreal C++: frame-critical movement, collision, animation integration, combat, in-ring AI execution, referee state, replication and presentation hooks.
- Blueprint: presentation and designer-facing tuning; avoid giant authoritative gameplay graphs.
- Enhanced Input, CharacterMovementComponent, UMG, Gameplay Tags, and incremental GAS adoption for abilities/costs/attributes. Initialize attributes/abilities correctly for AI, player possession and replicated lifecycle. Keep one authoritative resource state.
- Python: pure world simulation, progression, scouting/reference math, rankings, career rules and backend workers. FastAPI/Pydantic for the eventual API; PostgreSQL/SQLAlchemy/Alembic for official online state. Pin compatible versions after audit.
- Offline persistence: local SQLite/snapshots through a verified UE-compatible integration, with atomic checkpoints, backups and tested migrations.
- Start with a modular backend and a simple worker only when needed. Use a transactional outbox for reliable integration events. Do not create Kafka, a service mesh, numerous microservices, or a large admin app before measurements justify them. Redis is optional transient infrastructure, never canonical truth.
- Python must not run on each combat/rendered frame. Unreal's editor Python tooling is not the shipped gameplay runtime.
- Before relying on Python for shipped offline careers, prove a self-contained version-pinned local worker on a clean Windows machine without Python installed, internet, admin privileges, or external services. Prefer bounded JSON stdin/stdout requests at calendar actions during the spike. If packaging fails the gate, document a deliberate compiled-local alternative and its cross-language tests. P1 combat must not wait on this experiment.
- Reuse existing repository layout when present; otherwise use apps/game-unreal, apps/backend-api, apps/world-worker, packages/domain-python, packages/contracts, tests/golden, and docs. Do not scaffold empty services solely to fill this layout.

COMBAT AND NETWORKING FROM THE FIRST PLAYABLE MILESTONE
Build the combat model for standalone and server-authoritative play together. Prove two-player networking in P1; public matchmaking, hosting and ranked operations are later gates. This early proof does not make offline play service-dependent.

Implement orthodox/southpaw stances, eight-way footwork, circling, pivots, distance/reach management, ring cut-offs and boundaries. Start with a stable broadcast camera that frames both fighters. Add alternative cameras only after the core interaction works.

Attacks: jab/cross, lead/rear hooks and uppercuts, head/body variants, normal/power attacks and combinations. Define startup/active/recovery windows, trajectory, reach, stamina cost, impact and cancel rules in versioned data.

Defense: high/low and directional guard, parries, slips, ducks, rolls, pulls and counter windows. Clinch/break/referee intervention follows stable basic contact. Feints may be introduced after attack commitment and reaction rules can be measured.

Use server-evaluated swept contact through attack-active windows against head, body and guard volumes. Check guard interception; prevent punch tunneling, impossible range and repeated damage per attack ID/target. Debug trajectories can drive placeholders, but final contact must be validated against authored boxing motion and glove sockets. Do not use the full character capsule as the finished damage target.

Required skeletal pose/socket updates must run correctly on non-rendering dedicated servers. Do not rely solely on client notifies or render-driven animation ticking. Explicitly handle movement/root-motion interactions and action interruption.

The server owns attack legality, hit confirmation, resources, impact, referee state, scorecards, clocks and result. Clients send intent with sequence/time context, never authoritative hits/damage/stamina/wins. Validate ownership, timing, range, state, rate and affordability. Predict local response where supported and reconcile server outcomes; deduplicate predicted/confirmed cosmetics. Avoid reliable RPC floods.

Use normal UE networking initially; do not promise full rollback. Evaluate bounded historical hit validation after measuring latency, clamping timestamps to server-trusted limits and documenting attacker/defender tradeoffs. Choose one version-compatible session interface and local implementation first. EOS/Steam, WAN connectivity/relay and account setup require separate tested configuration.

Create a dedicated-server path for official online fights and check source-build/packaging requirements. Listen-server prototypes are unranked. Test separate-process clients, joining/leaving, packet loss, observed RTT, reconnects, and server failure. Do not label a local two-window test a verified internet service.

STAMINA, CONTACT DAMAGE AND PHYSIOLOGY
Model short-term burst energy, long-term fatigue, guard endurance, conditioning/readiness and body-damage effects as distinct connected concepts.

Punches, explosive movement and defense have tunable time-based costs. Power attacks and misses generally cost more than efficient jabs. Repeated combinations build fatigue. Activity reduction permits delayed recovery; fatigue lowers the recoverable ceiling. Between-round recovery is partial. Body damage alters recovery and output. Low energy changes punch speed/power, explosive movement and defensive recovery without fully immobilizing the boxer.

Separate head/body accumulated damage, acute stun/balance, facial injury appearance, and long-term career wear. Use frame-rate-independent resource updates and explicit exhausted-action behavior. Expose tuning curves and a developer overlay for costs, recovery, fatigue and impact inputs. These are gameplay approximations to calibrate, not validated medical claims.

Resolve contact quality using range, angle, trajectory/velocity class, balance, guard, target exposure, attacker readiness and defender durability. Player timing, spacing and defense must matter. No button-mash damage multiplier or permanent toughness reward from repeatedly taking punishment.

FLASH KOs, KNOCKDOWNS AND RULES
Distinguish brief stun, recoverable flash knockdown, counted knockdown, recovery, flash KO, referee TKO, doctor stoppage and corner retirement.

A severe clean counter may cause a flash KO before a general damage bar is depleted. Gate eligibility behind contact severity and exposure; blocked/grazing routine punches cannot trigger arbitrary KO rolls. If probability is used, it is tightly bounded, versioned, server-rolled and logged after eligibility checks. Provide reproducible development-only scenarios; exclude forced-KO controls from official builds.

Use explicit legal state transitions for pre-bout, active round, bell, break, count, recovery, stoppage and completion. Prevent post-stoppage hits and duplicate results. Address punches around the bell, knockdown timing, simultaneous stoppages and server failure through ruleset fixtures.

Default judging is 10-point must with configurable rules. Implement clean effective scoring events, knockdowns/fouls, and three judge cards. Do not score ineffective volume or movement aggression as automatically superior to clean boxing. Explain UD/SD/MD/draw outcomes from cards; distinguish KO/TKO/retirement/DQ/no-contest/technical outcomes where supported.

REGIONAL FACE SWELLING, CUTS AND BLOOD
Preserve Brian's explicit requirements; these are not optional omissions.
- Track left/right brow/eye/cheek, nose and mouth separately for bruising, cuts and swelling.
- Drive authored mesh-specific swelling morphs and validate naming, rig/topology, imported targets and LOD coverage. Missing assets must show a clear development diagnostic and degrade safely.
- Use validated face-local/UV material masks for bruises/cuts, brief Niagara blood/sweat impacts and bounded decals. Avoid sliding face marks or unlimited accumulation.
- Wounds persist through the bout; only configured treatment works between rounds. New bouts/practice reset according to the mode's rules. Career injury/wear is a separate model.
- Replicate persistent regional state and confirmed impact events, rebuilding presentation on clients and reconnects. Never replicate every particle or render cosmetic blood on headless servers.
- Include blood/reduced-effect settings without changing competitive damage. Doctor stoppage thresholds are server-owned and distinct from arbitrary cosmetic particle counts.
- Prototype logic/debug visualization first; realistic appearance remains asset-dependent. Do not claim authored swelling or production animation exists when placeholders are used.

AI AND TRAINING
Use a strategic plan, tactical decisions and legal motor inputs. Start with an inspectable C++ utility/state-machine implementation. Styles: out-boxer, pressure fighter, counterpuncher initially; later blend boxer-puncher, swarmer and slugger tendencies.

AI has limited information, reaction delay, error rates, pacing and fatigue. Difficulty improves decisions/consistency, not hidden damage/health boosts or future-input access. Personalities can later affect risk, adaptability, discipline, confidence and coachability.

Training uses the actual boxing controls for bag, mitts, defense, conditioning and controlled sparring. Later film study builds opponent familiarity. Give immediate useful feedback on range, guard openings, clean counters, missed punches and stamina waste.

Free practice/exhibitions never modify official ranked records. Career camp sessions have finite slots, intensity/recovery tradeoffs, diminishing returns, coach fit and injury exposure. Coach-delegated training stays viable; repeated manual drills cannot grant unlimited talent. Identify explicitly which career sparring sessions can cause persistent consequences.

FIGHTER STATE AND PROGRESSION
Keep these separate:
1. TrueAbility: causal fighter capabilities/potential.
2. ObservedScoutingEstimate: imperfect inferred assessment, including confidence.
3. CurrentReadiness: conditioning, fatigue, rust, weight/camp preparation.
4. InjuryBurden and long-term wear.
5. TacticalFamiliarity and style/personality preferences.

Internal ability values may be normalized 0–1 and displayed 0–100. Map power, speed, durability, defense, stamina, sustainable output/activity, ring IQ, finishing ability, damage resistance and technical consistency to explicit mechanics. Add accuracy, footwork, recovery/chin/cut resistance as defined subattributes instead of duplicate independent knobs.

A knockout victory can update public belief about power, not automatically create power. Training, recovery, aging, rehabilitation and injury cause ability changes. Potential is multidimensional; temporary readiness is not permanent talent. Inactivity primarily affects preparation/rust; irreversible aging uses separate versioned curves.

Fighter creation includes fictional identity, appearance, height/reach, stance, division and background. Use constrained canonical body geometry for competitive classes; cosmetics cannot change hitboxes. Start with presets and a small roster. Face scanning is deferred.

RATINGS, RECORDS, RANKINGS AND BELTS
Do not conflate:
- Fighter ability/scouting ratings.
- Player matchmaking skill rating (MMR).
- Fighter career merit ranking.
- Sanctioning-body title eligibility and champion status.

For the first player ladder, use clearly labeled configurable Elo: initial 1000, K=32, result 1/0.5/0, expected score 1/(1+10^((opponent-own)/400)). Update both players from pre-bout ratings with a defined rounding policy. No untested KO bonus. Mark provisional players.

Career merit rankings use a separate transparent, versioned policy based on eligible recent results, opponent quality, activity and division. Validate tie-breaking and avoid self-reinforcing/circular scoring. Each published list stores its inputs, as-of time and explanation; historical rankings cannot use future information. Never call our algorithm BoxRec's algorithm.

Build an original BoxRec-inspired records UI with sortable compact rows, fighter profiles and bout history. Columns: rank, fighter, country, division, rating/ranking score with clear label, W-L-D, KO/TKO wins, stoppage-win percentage with stated denominator, activity and recent form. Add division/body/geography/name filters, pagination and controller navigation.

Profiles include portrait, bio, stance, height/reach, attributes or scouted estimates, readiness visibility policy, opponent links, cards, injuries disclosed by mode, rating history and title lineage. Use our own fictional data and branding, not scraped assets/data.

Titles support local/state/regional/national/world structures through configurable fictional organizations. Geographic eligibility, mandatories, defenses, vacations, retirement, draws, no-contests, eliminators and unification are explicit rules. A state belt is not a mandatory prerequisite for world contention. Undisputed is derived from the configured major belts at the same time, not a manually assigned badge. Reserve title/fighter eligibility transactionally to prevent duplicate bookings/holders.

DOMAINS, UNIVERSES AND TIME
Competition domain TRADITIONAL/VR is distinct from device platform PC/console. Use immutable domain_id and universe_id on official entities. Local/offline, private league, normalized ranked and official career universes are separate authority pools. Accounts/approved cosmetics may be shared; competitive fighter state cannot cross into another pool automatically.

Normalized Ranked and Online Universe have separate record/rating policies. A bout updates only its explicitly authorized ledgers, never both by assumption. Offline save editing must not become a route into official online stats. Do not promise tamper-proof local offline saves.

Offline calendar advances by player action; official worlds use one authoritative scheduler. Separate wall-clock occurred_at from integer world_tick/calendar time. Lock/version career transitions so two competitions/workers cannot train, age or book the same canonical fighter twice. Freeze the match's pre-fight state and prevent conflicting career changes until finalization/reservation release.

Start with a small seeded NPC world and a simple create/train/fight/result/save loop. Later advance NPC camps, bookings, bouts, recovery, careers, prospect debuts and retirements via scheduled events. Allow bounded fast-forward and cancelable long operations. Promoters/managers/trainers act on limited information; representation is a time-bound contract, not permanent ownership.

HEADLESS SIMULATION AND REUSE
Playable and headless bouts share rule names, versioned definitions and calibration targets. An aggregate round simulator is not animation/collision combat. Calibrate scorecards, stoppage rates, fatigue and style distributions against instrumented playable bouts without forcing frame-identical outcomes.

The supplied document names OnyxByte/fightvantage-platform and candidate files under legacy-riff/backend/app/libs. Those findings are unverified inputs, not proof that code exists in this workspace, is licensed for reuse, or passes tests.

If access is available, audit exact source commit, permissions/license, dependencies and tests before copying anything. Consider state schemas/constants, age/decay/smoothing utilities and monte_carlo_engine.py as candidates. Remove Supabase/auth/entitlement/scraping coupling from extracted pure math. Keep provenance, golden fixtures and independent final_bell packages. Replace historical-outcome-as-skill-growth mappings with causal progression. Use only as-of opponent context. If access is missing, document the dependency and implement clean contracts independently; do not fabricate source inspection.

REPLAY AND AUDIT CONTRACT
- Exact reproducibility applies to pinned headless/world rules with controlled RNG, input ordering, numeric policy and versions. Test it explicitly.
- Real-time UE combat uses authoritative result/event logs, state checkpoints and a compatible UE replay recording for reconstruction/review. Do not assume seed+inputs reconstructs animation/physics bit-for-bit across machines/builds.
- Store rules/model/build versions, pre-fight snapshot, resolved impacts, judge/referee decisions, RNG decisions when applicable, result hash and match identity. Hashes do not prove a client report is trustworthy; certification still requires server identity and authorization.
- Retain replay compatibility policy and storage limits. The FB-ENH-02 flash-KO explanation replay is required at its stated gate; broader replay editing/coaching tools remain later backlog. Neither implies deterministic rollback.

PERSISTENCE AND OFFICIAL RESULTS
Define versioned contracts for FighterTrueState, FighterReadiness, CombatIntent, FightResult, WorldEvent, RankingSnapshot, TitleReign and SaveManifest. IDs, domain/universe, event/cause IDs, schema/rules versions and timestamps must be explicit. Generate/validate Python and C++ payloads using shared schemas/fixtures.

Start with immutable finalized results, selected career events and snapshots; adopt broader event sourcing only when needed. Local save writes are atomic with a recoverable prior checkpoint and tested upgrade fixtures. No downgrade migration is assumed safe on production saves.

For official online fights:
1. Authenticate players and reserve eligible participants with an immutable pre-fight snapshot.
2. Issue match-scoped tokens to a trusted dedicated server.
3. Accept final results only from an authorized server assigned to that match, never the player client.
4. Validate participants, allowed state transition and rules versions.
5. Use transaction/version checks and a unique scoped match ID/idempotency key to finalize once.
6. Commit canonical records/rating changes and outbox events atomically. Update heavier ranking/news projections idempotently from that committed result, recording revision/order.
7. Permit audited corrections through explicit new revisions/compensating events, not silent mutation of history.

Use composite domain/universe foreign keys, not only unrelated UUID foreign keys from illustrative DDL. Serialize competing updates, reject stale results, and prevent the world scheduler from applying duplicate changes. Delivery can be at-least-once; observable effects must be applied once.

Distinguish voluntary quit, temporary network interruption and server failure. Log authoritative reason/evidence and use bounded reservation/grace rules. Network symptoms alone do not prove intentional quitting. Unrecoverable server failure must not invent a player loss. Define any forfeit effects on player MMR versus fighter career health/titles separately.

No secrets in clients. Keep server credentials scoped and rotateable. Add account moderation/reporting, replay/audit access, version compatibility and cost/backup monitoring at the online gate. Do not create external accounts, purchase services, publish builds, or contact the partner without user instruction.

VR AND OTHER DEFERRED SCOPE
Keep schema boundaries for a future VR universe. No VR-versus-controller official fights, migrated stats, shared titles or shared ladders. VR later gets its own motion validation, reach calibration, comfort/accessibility and platform budget. Console, VR, face scan, complex promotion economics, numerous sanctioning bodies and broad commercial services must not delay the first playable boxing proof.

ONE EXECUTION ROADMAP — EXIT GATES, NOT RELEASE PROMISES
P0: Audit, naming, shared contracts, owner map, build recipe and highest-risk asset/packaging spikes. Green build; both developers can reproduce setup.
P1: One ring, two placeholder fighters, one division, jab/cross/guard, footwork/camera, basic stamina, three-round flow and minimal AI. Standalone AND a two-client authoritative combat smoke test, with a private-room/challenge foundation. No public service needed.
P2: Full punch/defense set, stamina/damage/flash-KO tuning, regional facial presentation hooks, training and distinct AI styles. Include FB-ENH-01 signature-defense acceptance, FB-ENH-03 actionable coaching, and FB-ENH-02 authoritative finish markers. Verified asset integration and no punch-spam/contact exploit.
P3: Fighter presets, camp/readiness, small offline career/world, rankings and one fictional championship path. Include FB-ENH-04 scouting cards, FB-ENH-05 linked records/rank-change explanations, and the FB-ENH-02 player-facing flash-KO replay. Atomic save/load and clean-machine offline packaging; no cloud dependency.
P4: Authenticated dedicated-server online ranked, one initial queue/ruleset, persistent records, basic reporting and tested failure/result policies. Private rooms are available earlier. No public competitive release without this gate.
P5: Deliver FB-ENH-06 scheduled fight nights and basic private-league tools. Expand generational careers, geography/belts, rivalries, optional promotion depth and Online Universe as capacity/playtests justify. Long-run simulation, performance and content polish.
P6: Console/VR feasibility and separately scoped expansion after traditional combat/online quality is proven. VR state remains isolated.

Do not turn these gates into promised weeks/months before the team measures actual delivery. Prioritize the highest-risk animation/contact/networking proofs. Full ambitions are retained as backlog rather than forced into P1.

TESTS AND DEFINITION OF DONE
Add meaningful checks when their systems arrive: resource bounds/frame-rate effects, attack deduplication/guard interception, clean-hit KO eligibility, legal referee transitions/cards, Elo win/draw/void behavior, finalization retries/concurrency, domain isolation, historical as-of rankings, duplicate aging/bookings, title lineage, crash-safe saves and old-save upgrades.

Test two clients with measured representative RTT/loss, reconnection and server crash; verify identical official clock/stoppage/result and restored facial state. Measure frame times/server tick overruns on recorded hardware; 60 FPS is a target until demonstrated. Avoid repeatedly broadening tests when current acceptance already passes.

Headless tests pin model/build/RNG versions; progressively increase world simulation from small fixtures to long-career soak. Keep playtest telemetry on contact credibility, defensive counters, style viability, stamina spam and judging agreement. No hidden AI advantages.

At each completed increment: changed files, actual build/tests, measured limits, remaining manual asset/editor actions, and next task. Update docs/Progress.md. A generated class, mock leaderboard or local editor screenshot alone is not a completed production feature.

START NOW
Read the companion decisions if available, inspect the real workspace, and implement P0 plus the smallest compilable P1 increment. Add FB-ENH-01 through FB-ENH-06 to docs/FeatureStatus.md and the milestone backlog. Establish the next two bounded tasks, one per developer, with a shared contract and separate file ownership. Do not assume the partner's workspace is accessible. Continue independent work through routine decisions; stop dependent work only for a concrete unavailable tool/asset or a conflicting user requirement.
```

To resume: ask Cursor to read this file and `docs/Progress.md`, verify the current build, and continue the next incomplete task without recreating completed work.
