# Final Bell — Contract Registry

| Contract | Schema file | Version | C++ mirror | Python | Fixtures |
| --- | --- | --- | --- | --- | --- |
| FighterTrueState | `packages/contracts/schemas/fighter_true_state.schema.json` | 1.0.0 | TBD (save/career) | `final_bell.contracts` | `fighter_true_state.valid.json` |
| FighterReadiness | `packages/contracts/schemas/fighter_readiness.schema.json` | 1.0.0 | TBD | `final_bell.contracts` | (add valid fixture in FB-002b) |
| CombatIntent | `packages/contracts/schemas/combat_intent.schema.json` | 1.0.0 | `FFBCombatIntent` | `final_bell.contracts` | valid + bad_schema |
| FightResult | `packages/contracts/schemas/fight_result.schema.json` | 1.0.0 | TBD finalize payload | `final_bell.contracts` | `fight_result.valid.json` |
| WorldEvent | — | planned | — | — | P3 |
| RankingSnapshot | — | planned | — | — | P3 |
| TitleReign | — | planned | — | — | P3 |
| SaveManifest | — | planned | — | — | P3 |

## Compatibility rules

1. Bump `schema_version` only with owner+reviewer agreement and migration notes.
2. Reject unknown/mismatched versions at validation boundaries (Python tests + UE intent path).
3. Official results require `domain_id`, `universe_id`, `idempotency_key`, and authorized ledger list.
4. CombatIntent must never include damage, stamina outcomes, or win fields.
