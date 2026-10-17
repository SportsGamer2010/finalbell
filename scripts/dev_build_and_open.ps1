# Final Bell — build editor target and open the project (P1 auto-arena).
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $Root "apps\game-unreal\FinalBell.uproject"
$BuildBat = "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

Get-Process UnrealEditor, LiveCodingConsole -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

& $BuildBat FinalBellEditor Win64 Development "-Project=$UProject" -WaitMutex
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit $LASTEXITCODE" }

Write-Host "Build succeeded. Opening editor (PIE uses auto-arena on /Engine/Maps/Entry)..."
Start-Process -FilePath $Editor -ArgumentList "`"$UProject`""
