# Final Bell — FAB / free boxing asset shortlist

Do **not** invent `.uasset` files in git. Add assets only after you pull them in the Unreal Editor (Fab → Add to Project) and verify they load.

## Search queries (Fab)

1. `boxing` + filter **Free**
2. `heavy bag` / `punching bag` + Free
3. `boxing gloves` + Free
4. `boxing animation` + Epic skeleton

Fab free listings change often. Re-check price badges before adding.

## Strong candidates (as of research 2026-10-17)

| Priority | Listing | Why | Notes |
| --- | --- | --- | --- |
| 1 | Epic **Game Animation Sample** / free retargeted GASP packs on Fab | Free loco/combat-adjacent anims on mannequin | Not boxing-specific; good footwork base |
| 2 | Fab **Boxing Animation Pack** (67 MoCap, Epic skeleton) | Jab/cross/hooks/uppercuts/guards/slips | Likely paid — confirm Free filter; best anim match for Training |
| 3 | Fab **Boxing Anim Pack** (15 anims) | Smaller starter set | Confirm price |
| 4 | Fab gloves / bag props | Replace procedural cylinder bag + sphere gloves | Many glove listings are paid; prefer Free + UE5 |

## Free non-Fab fallbacks (if Fab Free is empty)

- RigModels free boxing gloves / heavy bag meshes (FBX) → import + reassign materials
- Keep current procedural bag until a Free Fab bag is added

## Recommended Editor workflow

1. Fab → search Free boxing / bag / gloves
2. Add to `FinalBell` project under `Content/ThirdParty/...`
3. Update `AFBHeavyBag` / glove static meshes to soft-path load those assets
4. Retarget boxing anims onto `ABP_Unarmed` or a new `ABP_Boxing`
5. Log paths in AssetChecklist as **In-editor** only after you personally add them

## License

Prefer Epic/Fab Free licenses that allow commercial game use. Avoid AI-only packs with unclear rights for competitive release.
