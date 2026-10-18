# Defensive mechanics

1. Read the existing high guard, weave, hook facing lock, and created-angle window in `FBBoxerCharacter.cpp`.
2. Add slip, duck, parry, or counter as server-owned contact changes. A missed punch can open a counter. It must not be a client-side damage call.
3. Preserve opponent-facing rotation and the ring clamp.
4. Build the editor target and say whether a playtest was run.
