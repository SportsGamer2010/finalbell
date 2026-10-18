#Requires -Version 5.1
<#
.SYNOPSIS
  Run the Final Bell orchestrator with a real Python, skipping the Store alias.
#>
[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ErrorActionPreference = "Stop"
$roots = @(
    (Join-Path $env:LOCALAPPDATA "Programs\Python"),
    "C:\Program Files\Python312"
)
$python = $null
foreach ($root in $roots) {
    if (-not (Test-Path $root)) { continue }
    $found = Get-ChildItem -Path $root -Filter python.exe -Recurse -Depth 2 -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($found) { $python = $found.FullName; break }
}
if (-not $python) {
    $command = Get-Command python -ErrorAction SilentlyContinue
    if ($command -and $command.Source -notmatch "WindowsApps") { $python = $command.Source }
}
if (-not $python) {
    Write-Error "Real Python 3.11+ was not found. Run Tools/Setup/Install-FinalBell-Toolchain.ps1 first."
}
$entry = Join-Path $PSScriptRoot "fb_orchestrator.py"
& $python $entry @Args
exit $LASTEXITCODE
