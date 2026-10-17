# Final Bell — Controller setup (DualSense / PS5)

## What the Output Log showed

| Log line | Meaning |
| --- | --- |
| `RegisterDevice ... HardwareDeviceIdentifier=KBM` | Only keyboard/mouse registered |
| `Registering Device Callback for GameInputKind: 'Unknown'` | Windows GameInput defaults had **gamepad/controller processing off** |
| `FinalBell: 1 connected input device(s)` | DualSense never reached Unreal |

## Fix applied in repo

`Config/DefaultInput.ini` now enables on Windows:

- `bProcessGamepad=True`
- `bProcessController=True`
- `bProcessRawInput=True`
- DualSense / DualSense Edge vendor+product IDs (Sony `0x054C`)

**You must fully quit and reopen the Unreal Editor** after this config change (GameInput reads settings at startup).

## Test steps

1. Quit Unreal completely (also close Live Coding console).
2. Plug DualSense in with **USB**.
3. Reopen `FinalBell.uproject`.
4. Play on `Ring_P1`, click the game viewport.
5. Output Log should show `GameInput flags: Gamepad=1 Controller=1` and **more than 1** connected device.
6. Left stick moves blue; □ / ✕ jabs.

## Two controllers (2-player PIE)

Yes — both should work at once after the `FBGameViewportClient` change:

- Pad 1 → host window (blue)
- Pad 2 → client window (red)
- Keep **Route Gamepad to Second Window** unchecked
- Play with **Number of Players: 2** / Listen Server

## Fallback if still 1 device

Use DSX (ViGEm is already on this machine) to expose an Xbox 360 virtual pad, then Play again.
