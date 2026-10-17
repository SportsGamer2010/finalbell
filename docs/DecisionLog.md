# Final Bell — Decision Log

| Date | ID | Decision | Rationale | Status |
| --- | --- | --- | --- | --- |
| 2026-10-17 | D-001 | New monorepo `final-bell` at `Projects/final-bell`; no existing game rename | Audit found only Cursor package, no `.uproject` game | Active |
| 2026-10-17 | D-002 | Target UE **5.8** matching installed engine | Do not upgrade/downgrade without need | Active |
| 2026-10-17 | D-003 | Scaffold from Blank C++ pattern + custom boxing classes (not ThirdPerson combat variant) | Avoid unrelated template combat systems; keep boxing authority explicit | Active |
| 2026-10-17 | D-004 | Enable Enhanced Input + GameplayAbilities plugins early; GAS attributes deferred | Incremental GAS adoption per master prompt | Active |
| 2026-10-17 | D-005 | P1 net proof uses IpNetDriver / listen-server; EOS/Steam later | Master prompt + review | Active |
| 2026-10-17 | D-006 | FightVantage untreated as dependency | Repo not in workspace; license/tests unverified | Active |
| 2026-10-17 | D-007 | Contract schemas start at `1.0.0` with domain/universe fields | Review composite key requirement | Active |
| 2026-10-17 | D-008 | Do not fabricate `.uasset`/`.umap` | Editor ownership required | Active |
| 2026-10-17 | D-009 | Install VS 2022 BuildTools + Win SDK + NetFx 4.8.1 Dev Pack on this machine for P0 | Required for UBT Win64 | Active |
| 2026-10-17 | D-010 | `PublicIncludePaths.Add(ModuleDirectory)` for FinalBell module | Fixes `Combat/`-style includes under UE 5.8 | Active |
