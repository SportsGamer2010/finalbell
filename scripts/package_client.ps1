# Final Bell — cook/stage/archive a Win64 Development game client for partner shares.
param(
	[ValidateSet("Development", "Shipping")]
	[string]$Config = "Development",
	[string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
	[string]$ArchiveDir = ""
)
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $Root "apps\game-unreal\FinalBell.uproject"
if (-not $ArchiveDir) {
	$ArchiveDir = Join-Path $Root "apps\game-unreal\Packaged\Win64"
}
$UAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $UAT)) { throw "RunUAT not found: $UAT" }
if (-not (Test-Path $UProject)) { throw "Missing uproject: $UProject" }

New-Item -ItemType Directory -Force -Path $ArchiveDir | Out-Null

Write-Host "Packaging FinalBell Win64 $Config -> $ArchiveDir"
& $UAT BuildCookRun `
	"-project=$UProject" `
	-noP4 `
	-platform=Win64 `
	"-clientconfig=$Config" `
	-build `
	-cook `
	-stage `
	-pak `
	-archive `
	"-archivedirectory=$ArchiveDir" `
	-utf8output

if ($LASTEXITCODE -ne 0) { throw "Package failed with exit $LASTEXITCODE" }

$Exe = Get-ChildItem -Path $ArchiveDir -Recurse -Filter "FinalBell.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($Exe) {
	Write-Host "OK: $($Exe.FullName)"
	Write-Host "Zip the archive folder and share with your partner. Then use host_listen_server.ps1 / join_listen_server.ps1"
} else {
	Write-Warning "Build finished but FinalBell.exe not found under $ArchiveDir — inspect UAT logs."
}
