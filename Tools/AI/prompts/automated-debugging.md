# Automated debugging

1. Read the newest log under `apps/game-unreal/Saved/Logs` and `Tools/AI/logs`.
2. Match the failure to the class that owns it: ring bootstrap, boxer, camera, combat, or input.
3. Do not delete `Saved/`, `Intermediate/`, or `Binaries/` unless the user asks.
4. After a fix, cite the command and the result. If the editor had the project locked, say the build did not run.
