# Open Final Bell directly with the installed UE 5.8.3 editor (skips launcher version picker).
$ErrorActionPreference = "Stop"
$UProject = Join-Path (Split-Path -Parent $PSScriptRoot) "apps\game-unreal\FinalBell.uproject"
$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $Editor)) { throw "Unreal Editor not found at $Editor" }
if (-not (Test-Path $UProject)) { throw "Project not found at $UProject" }
Start-Process -FilePath $Editor -ArgumentList "`"$UProject`""
