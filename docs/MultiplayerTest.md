# Final Bell — Local 2-player / second view test

Yes: you can test a second player view on one PC without packaging.

## Option A — PIE listen server (recommended)

1. Open `Ring_P1` in Unreal Editor.
2. Project defaults are already **2 players / Listen Server**. Confirm in the Play dropdown if needed:
   - **Number of Players:** `2`
   - **Net Mode:** `Play As Listen Server`
3. Click **Play**.
4. (If you Play with **1** player, only pad 1 works — pad 2 has no fighter.)
5. Two game windows open:
   - Window 1 = host (blue corner) → **controller 1**
   - Window 2 = client (red corner) → **controller 2**
6. Leave **Route Gamepad to Second Window** unchecked (Play dropdown → Advanced Settings).

**Two controllers**
- Yes — both pads should work at once.
- Pad 1 → host / blue. Pad 2 → client / red.
- Keyboard/mouse still follow the focused window (WASD / J / K / Shift, Q/E orbit).
- Log should show `PIE instance 0 bound to controller/user 0` and `PIE instance 1 bound to controller/user 1`.
- Fully quit/reopen the editor after pulling the GameViewportClient change, then Play with 2 players.

## Option B — split screen (same window)

In Play settings, enable **Use Viewport** / multiplayer window mode if you prefer one window with two views. Listen-server + 2 players is clearer for networking checks.

## What should happen

| Setup | Blue | Red |
| --- | --- | --- |
| 1 player PIE | You | AI/dummy placeholder |
| 2 player PIE | Human host | Human client |

Each human gets their **own orbit camera** looking at their opponent.

## What the server owns

Hits, damage, stamina, attack phases, and bout clock are authority-side. Clients send intents only.

## Next networking checks

- Both see glove extend on jab/cross
- Block raises gloves and reduces damage
- Damage applies once per attack id
- Disconnect/reconnect comes later (P1 smoke first)
