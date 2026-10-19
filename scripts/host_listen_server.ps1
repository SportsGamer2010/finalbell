# Final Bell — start a listen server on port 7777 (packaged exe preferred, else editor -game).
param(
	[int]$Port = 7777,
	[string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
	[string]$PackagedRoot = "",
	[string]$Map = "/Game/Maps/Ring_P1"
)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $Root "apps\game-unreal\FinalBell.uproject"
if (-not $PackagedRoot) {
	$PackagedRoot = Join-Path $Root "apps\game-unreal\Packaged\Win64"
}

$Exe = Get-ChildItem -Path $PackagedRoot -Recurse -Filter "FinalBell.exe" -ErrorAction SilentlyContinue |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1

$Args = @()
if ($Exe) {
	Write-Host "Hosting packaged: $($Exe.FullName)"
	$WorkDir = $Exe.DirectoryName
	$Args = @(
		"${Map}?listen",
		"-Port=$Port",
		"-log",
		"-WINDOWED",
		"-ResX=1280",
		"-ResY=720"
	)
	Start-Process -FilePath $Exe.FullName -ArgumentList $Args -WorkingDirectory $WorkDir
} else {
	$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
	if (-not (Test-Path $Editor)) { throw "No packaged FinalBell.exe under $PackagedRoot and editor missing: $Editor" }
	Write-Host "No package found — hosting via editor -game (partner needs matching build/git)."
	$Args = @(
		"`"$UProject`"",
		"${Map}?listen",
		"-game",
		"-Port=$Port",
		"-log",
		"-WINDOWED",
		"-ResX=1280",
		"-ResY=720"
	)
	Start-Process -FilePath $Editor -ArgumentList ($Args -join " ")
}

$TailIp = $null
try { $TailIp = (& tailscale ip -4 2>$null | Select-Object -First 1) } catch {}
if ($TailIp) {
	Write-Host "Tailscale IPv4: $TailIp"
	Write-Host "Partner: .\scripts\join_listen_server.ps1 -HostAddress $TailIp"
	Write-Host "Or in-game console: FBJoin $TailIp"
} else {
	Write-Host "Share your Tailscale/LAN IPv4. Partner: join_listen_server.ps1 -HostAddress <ip>"
	Write-Host "In-game console on host: FBRoom  |  on client: FBJoin <ip>"
}
