# Final Bell — Build Recipe (Windows)

## Required tools

1. **Unreal Engine 5.8** (installed): `C:\Program Files\Epic Games\UE_5.8`
2. **Visual Studio 2022** with Desktop development with C++ and the workloads/components Unreal 5.8 documents for Windows
3. **Git for Windows** + **Git LFS**
4. **Python 3.11+** (real install; not the Microsoft Store alias) for contract tests

## Game compile

```powershell
cd C:\Users\Shadow\Projects\final-bell\apps\game-unreal
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FinalBellEditor Win64 Development -Project="$PWD\FinalBell.uproject" -WaitMutex
```

Or: right-click `FinalBell.uproject` → Generate Visual Studio project files → open `.sln` → build Development Editor.

## Packaged client (remote partner share)

```powershell
cd C:\Users\Shadow\Projects\final-bell
.\scripts\package_client.ps1
```

Archive: `apps/game-unreal/Packaged/Win64/`. Zip and share privately. Both machines must use the **same** package.

## Listen-server host / join (port 7777)

```powershell
# Host (after packaging, or falls back to editor -game)
.\scripts\host_listen_server.ps1

# Partner
.\scripts\join_listen_server.ps1 -HostAddress 100.x.y.z
```

In-game console: `FBHost`, `FBJoin <ip>`, `FBRoom`. Full remote checklist: [RemotePartnerMultiplayer.md](RemotePartnerMultiplayer.md). Local PIE first: [MultiplayerTest.md](MultiplayerTest.md).

Default game port is set in `Config/DefaultEngine.ini` (`[URL] Port=7777`).

## Dedicated server (later packaging)

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FinalBellServer Win64 Development -Project="$PWD\FinalBell.uproject" -WaitMutex
```

## Contract tests

```powershell
cd C:\Users\Shadow\Projects\final-bell\packages\domain-python
python -m pip install -e ".[dev]"
python -m pytest ..\..\tests\golden -q
```

## Two-client smoke

1. Local: 2-player PIE listen server ([MultiplayerTest.md](MultiplayerTest.md))
2. Remote: host `host_listen_server.ps1`, client `join_listen_server.ps1 -HostAddress <tailscale-ip>`
3. Record observed RTT/loss; confirm identical stamina/hit outcomes for the same intents
4. Do **not** label Tailscale listen-server as a verified public internet service
