# Combat feature

1. Route the request, then read `Tools/AI/config/combat_extension_points.json`.
2. Read `FBCombatComponent` and `FBBoxerCharacter` before editing.
3. Clients submit `FFBCombatIntent`. The server owns contact, damage, stamina, and the result.
4. Keep the change small enough to compile. Record the `Build.bat` result. Do not claim the punch works without that build or a playtest.
