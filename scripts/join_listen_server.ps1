# Final Bell — join a remote listen server (packaged exe preferred, else editor -game).
param(
	[Parameter(Mandatory = $true)]
	[string]$HostAddress,
	[int]$Port = 7777,
	[string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
	[string]$PackagedRoot = ""
)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $Root "apps\game-unreal\FinalBell.uproject"
if (-not $PackagedRoot) {
	$PackagedRoot = Join-Path $Root "apps\game-unreal\Packaged\Win64"
}

$Target = $HostAddress
if ($Target -notmatch ":\d+$") {
	$Target = "${HostAddress}:${Port}"
}

$Exe = Get-ChildItem -Path $PackagedRoot -Recurse -Filter "FinalBell.exe" -ErrorAction SilentlyContinue |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1

if ($Exe) {
	Write-Host "Joining packaged -> $Target"
	Start-Process -FilePath $Exe.FullName -ArgumentList @($Target, "-log", "-WINDOWED", "-ResX=1280", "-ResY=720") -WorkingDirectory $Exe.DirectoryName
} else {
	$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
	if (-not (Test-Path $Editor)) { throw "No packaged FinalBell.exe and editor missing: $Editor" }
	Write-Host "No package found — joining via editor -game -> $Target"
	Start-Process -FilePath $Editor -ArgumentList "`"$UProject`" $Target -game -log -WINDOWED -ResX=1280 -ResY=720"
}

Write-Host "If the game is already running, use console instead: FBJoin $HostAddress"
