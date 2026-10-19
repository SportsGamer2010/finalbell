# Final Bell — Remote partner multiplayer (P1)

There is **no public matchmaking server** yet (EOS/Steam + dedicated are a later gate). Both of you need a **matching client build**. One person **hosts a listen server**; the other **joins by IP** on port **7777**.

```
Partner client  --Tailscale/LAN-->  Host listen server (FinalBell.exe or editor -game)
```

## Efficiency ranking

1. Same-PC dual PIE — [MultiplayerTest.md](MultiplayerTest.md) (do first)
2. Packaged client + Tailscale listen-server (this doc)
3. Shared git + both build `FinalBellEditor` (partner is a developer)
4. Public dedicated / Steam / EOS — later, not for this smoke

## Prerequisites

- Same build (same git commit --trailer "Co-authored-by: Cursor <cursoragent@cursor.com>" or same packaged zip)
- Both install [Tailscale](https://tailscale.com/) and join the **same tailnet** (or use LAN)
- Windows Firewall allows Final Bell / UnrealEditor inbound UDP/TCP on the Tailscale interface (or allow the app when prompted)

## Package a Windows client (host machine)

```powershell
cd C:\Users\Shadow\Projects\final-bell
.\scripts\package_client.ps1
```

Output defaults to `apps/game-unreal/Packaged/Win64/`. Zip that folder and share privately (Drive / zip / itch). Partner extracts to a stable path.

If packaging is blocked, both can use editor `-game` instead (requires UE 5.8 on both PCs) via the host/join scripts below.

## Host steps

1. Note your Tailscale IPv4 (`100.x.y.z`) from the Tailscale app or `tailscale ip -4`.
2. Start listen server:

```powershell
.\scripts\host_listen_server.ps1
```

Or, in a running game console (`~`):

```
FBHost
FBRoom
```

3. Tell your partner: `FBJoin <your-tailscale-ip>` (port 7777 default).

## Partner (client) steps

```powershell
.\scripts\join_listen_server.ps1 -HostAddress 100.x.y.z
```

Or launch any matching client and in console:

```
FBJoin 100.x.y.z
```

Legacy UE console also works: `open 100.x.y.z:7777`

## Checklist

1. Same build version
2. Host listen server running; firewall/Tailscale OK
3. Partner joins `host:7777`
4. Both see same jab/block/damage; note rough RTT
5. Log result in [Progress.md](Progress.md) under FB-006 — do **not** call this a verified public internet service

## Console commands

| Command | Who | Effect |
| --- | --- | --- |
| `FBHost [port]` | Host | ServerTravel `/Game/Maps/Ring_P1?listen` |
| `FBJoin <ip[:port]>` | Client | ClientTravel to host |
| `FBRoom` | Host | Print room code + preferred host IP |

## What not to do yet

- Dedicated cloud server (`FinalBellServer`) — not required for 1v1 P1
- Steam/EOS sessions — P4
- Sending only a “server” with no client — partner still needs a matching client
