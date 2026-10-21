# Final Bell — Private match testing

This is a two-player listen server. The join text is the host address and port, for example `100.64.0.2:7777`. The menu does not show a room code. A code cannot be resolved until Epic Online Services is signed in, and this project has no Developer Portal product, sandbox, deployment, or client credentials.

Offline VS AI, Local VS, Career, and Training do not sign in to EOS.

## What is still required in the Epic Developer Portal

EOS relay, so two home networks can connect without port forwarding, is not finished. `OnlineSubsystemEOS` is enabled in `FinalBell.uproject`. The game net driver is still `IpNetDriver` on purpose: changing it before a login exists would stop offline play.

To turn on EOS later:

1. Create a product in the Epic Developer Portal for Unreal Engine 5.8.
2. Add a sandbox and a deployment.
3. Create a client policy that allows user presence and sessions. Do not assume device-ID login works. Use the portal's account login for testers.
4. Put the product id, sandbox id, deployment id, client id, and client secret in a local `DefaultEngine.ini` artifact. Do not commit secrets.
5. Set `DefaultPlatformService=EOS` only after a login succeeds in a test.
6. Point the online match at `OnlineSubsystemEOS.NetDriverEOS`, and leave `IpNetDriver` as the fallback for offline play.
7. Confirm a session search can find a lobby by an attribute before any short code is shown in the menu.

Until that login works, share the host address over a VPN such as Tailscale, or over a LAN. That path still needs the host to be reachable. It is not proof of EOS relay.

## Package a Windows build

From the repo root, on a machine with UE 5.8 installed:

```powershell
.\scripts\package_client.ps1
```

The script writes `apps/game-unreal/Packaged/Win64/`. Zip that entire folder. Include `FinalBell.exe`, the `FinalBell` folder next to it (the pak and content), every DLL, and any prerequisite installer the script places beside the exe. Do not send the source tree or the editor.

Your friend unzips the folder and runs `FinalBell.exe`. Unreal Editor is not required. If Windows asks for the Visual C++ runtime and the package included `Engine\Extras\Redist\en-us\vc_redist.x64.exe`, run that installer once.

## Host, share, and join

1. Both people use the same zip. The ring menu must list **ONLINE PRIVATE MATCH**. A build that only lists 1V1 Multiplayer is the older menu.
2. The host opens the game, chooses **ONLINE PRIVATE MATCH**, then **HOST MATCH**.
3. The game copies `host:port` to the clipboard and opens fighter select. Read that address from the status line if the clipboard was overwritten.
4. The friend opens Join Match, pastes the address, and chooses Join.
5. The host is the left fighter. The friend is the right fighter. Each person confirms only their own side. The host starts the bout after both confirms.
6. Cancel or Back leaves the match. If the host leaves, the friend is returned to the menu.

Local VS uses two controllers on one computer. The second person presses Start on their controller before they can choose. VS AI uses one controller. An extra controller does not pick the AI.

## Logs

Packaged logs are under:

`%LOCALAPPDATA%\FinalBell\Saved\Logs\FinalBell.log`

If the folder name differs, look in `%LOCALAPPDATA%` for `FinalBell` or `FinalBell.exe`. Send that log with the address that was shared and whether the failure was host, join, select, or the bout.

## Checks not done in this change

- Two controllers at once, mouse ownership, and an extra pad in VS AI were not played.
- A guest on another computer was not connected.
- Invalid addresses are rejected in code. A full room sends the third player back to the menu. Those paths were not played.
- Latency simulation and two internet connections were not run.
- Editor PIE is not evidence that EOS relay works.
