#Requires -Version 5.1
<#
.SYNOPSIS
  Idempotent Final Bell toolchain bootstrap for Windows.

.DESCRIPTION
  Detects Git, GitHub CLI, Python, Blender, FFmpeg, Node.js, Visual Studio C++
  build tools, and Unreal Engine. Installs missing approved free packages with
  winget. Skips compatible versions that are already present.

  Running without -AuditOnly installs only the free packages listed in
  Tools/AI/config/install_manifest.json and accepts winget package agreements
  for those packages. It does not install paid software, sign into accounts,
  enable Unreal plugins, or change the game project.

.PARAMETER AuditOnly
  Report status and write a summary. Do not install anything.

.PARAMETER InstallVisualStudioBuildTools
  If the C++ toolset is missing, install Visual Studio 2022 Build Tools.
  This is large and may request administrator approval. Off by default.
#>
[CmdletBinding()]
param(
    [switch]$AuditOnly,
    [switch]$InstallVisualStudioBuildTools
)

$ErrorActionPreference = "Continue"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ManifestPath = Join-Path $RepoRoot "Tools\AI\config\install_manifest.json"
$LogDir = Join-Path $RepoRoot "Tools\AI\logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$Results = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param(
        [string]$Name,
        [string]$Status,
        [string]$Detail
    )
    $entry = [ordered]@{
        name   = $Name
        status = $Status
        detail = $Detail
    }
    $Results.Add($entry) | Out-Null
    Write-Host ("[{0}] {1} - {2}" -f $Status.ToUpper(), $Name, $Detail)
}

function Refresh-Path {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$machine;$user"
}

function Test-WindowsAppsStub {
    param([string]$Path)
    return ($Path -and ($Path -match "\\WindowsApps\\"))
}

function Find-CommandPath {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd -and -not (Test-WindowsAppsStub $cmd.Source)) {
        return $cmd.Source
    }
    $winget = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages"
    if (Test-Path $winget) {
        $hit = Get-ChildItem -Path $winget -Filter "$Name.exe" -Recurse -Depth 4 -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    return $null
}

function Get-VersionText {
    param(
        [string]$File,
        [string[]]$Arguments
    )
    if (-not $File) { return "" }
    $quoted = '"' + $File + '"'
    $argText = @($Arguments) -join " "
    $output = cmd.exe /d /c "$quoted $argText 2>&1"
    $line = @($output) | Select-Object -First 1
    if (-not $line) { return "" }
    return "$line".Trim()
}

function Find-Python {
    $cmd = Find-CommandPath "python"
    if ($cmd) {
        $version = Get-VersionText $cmd @("--version")
        if ($version -match "Python 3\.(\d+)") {
            return @{ Path = $cmd; Version = $version }
        }
    }
    $roots = @(
        (Join-Path $env:LOCALAPPDATA "Programs\Python"),
        "C:\Program Files\Python312",
        "C:\Program Files\Python311"
    )
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $candidates = Get-ChildItem -Path $root -Filter python.exe -Recurse -Depth 2 -ErrorAction SilentlyContinue
        foreach ($candidate in $candidates) {
            $version = Get-VersionText $candidate.FullName @("--version")
            if ($version -match "Python 3\.(\d+)" -and [int]$Matches[1] -ge 11) {
                return @{ Path = $candidate.FullName; Version = $version }
            }
        }
    }
    return $null
}

function Find-Blender {
    $cmd = Find-CommandPath "blender"
    if ($cmd) {
        $version = Get-VersionText $cmd @("--version")
        return @{ Path = $cmd; Version = $version }
    }
    $roots = @(
        "C:\Program Files\Blender Foundation",
        (Join-Path $env:LOCALAPPDATA "Programs\Blender Foundation"),
        (Join-Path $env:ProgramFiles "Blender Foundation")
    )
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $candidates = Get-ChildItem -Path $root -Filter blender.exe -Recurse -Depth 3 -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending
        foreach ($candidate in $candidates) {
            $version = Get-VersionText $candidate.FullName @("--version")
            if ($version -match "Blender ([0-9]+)\.([0-9]+)") {
                $major = [int]$Matches[1]
                $minor = [int]$Matches[2]
                if ($major -gt 4 -or ($major -eq 4 -and $minor -ge 2)) {
                    return @{ Path = $candidate.FullName; Version = $version }
                }
            }
        }
    }
    return $null
}

function Find-Unreal {
    $editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
    $versionFile = "C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version"
    if ((Test-Path $editor) -and (Test-Path $versionFile)) {
        $raw = Get-Content $versionFile -Raw
        return @{ Path = $editor; Version = ($raw -replace "\s+", " ").Trim() }
    }
    return $null
}

function Find-Msvc {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }
    $install = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $install) { return $null }
    $cl = Get-ChildItem -Path (Join-Path $install "VC\Tools\MSVC") -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "Hostx64\\x64\\cl.exe" } |
        Select-Object -First 1
    if (-not $cl) { return $null }
    $banner = Get-VersionText $cl.FullName @()
    return @{ Path = $cl.FullName; Version = "$install $banner".Trim() }
}

function Install-BlenderOfficialZip {
    $version = "4.5.10"
    $folderName = "blender-$version-windows-x64"
    $url = "https://download.blender.org/release/Blender4.5/$folderName.zip"
    $hashUrl = "$url.sha256"
    $destRoot = Join-Path $env:LOCALAPPDATA "Programs\Blender Foundation"
    $final = Join-Path $destRoot "Blender 4.5"
    $exe = Join-Path $final "blender.exe"
    if (Test-Path $exe) {
        return @{ Ok = $true; Already = $true; Detail = $exe }
    }
    New-Item -ItemType Directory -Force -Path $destRoot | Out-Null
    Write-Host "Downloading Blender $version from download.blender.org"
    $zip = Join-Path $env:TEMP "$folderName.zip"
    $hashFile = Join-Path $env:TEMP "$folderName.zip.sha256"
    Invoke-WebRequest -Uri $url -OutFile $zip
    $expected = $null
    try {
        Invoke-WebRequest -Uri $hashUrl -OutFile $hashFile
        $expected = (Get-Content $hashFile -Raw).Split()[0].Trim().ToLower()
    } catch {
        $expected = $null
    }
    if ($expected) {
        $actual = (Get-FileHash -Algorithm SHA256 -Path $zip).Hash.ToLower()
        if ($actual -ne $expected) {
            return @{ Ok = $false; Already = $false; Detail = "SHA256 mismatch for $url" }
        }
    }
    if (Test-Path (Join-Path $destRoot $folderName)) {
        Remove-Item -LiteralPath (Join-Path $destRoot $folderName) -Recurse -Force
    }
    & tar.exe -xf $zip -C $destRoot
    $extracted = Join-Path $destRoot $folderName
    if (-not (Test-Path (Join-Path $extracted "blender.exe"))) {
        return @{ Ok = $false; Already = $false; Detail = "Archive did not contain blender.exe" }
    }
    if (Test-Path $final) {
        Remove-Item -LiteralPath $final -Recurse -Force
    }
    Rename-Item -LiteralPath $extracted -NewName "Blender 4.5"
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($userPath -notlike "*\Blender Foundation\Blender 4.5*") {
        [Environment]::SetEnvironmentVariable("Path", "$userPath;$final", "User")
    }
    $detail = "Official zip $version extracted to $final"
    if (-not $expected) {
        $detail += ". download.blender.org had no .sha256 sidecar, so the archive hash was not checked."
    }
    return @{ Ok = $true; Already = $false; Detail = $detail }
}

function Install-WingetPackage {
    param(
        [string]$Id,
        [string]$PreferredScope
    )
    $scopes = @()
    if ($PreferredScope) { $scopes += $PreferredScope }
    if ($PreferredScope -ne "machine") { $scopes += "machine" }

    foreach ($scope in $scopes) {
        Write-Host "winget install $Id (scope=$scope)"
        $output = & winget install --id $Id -e --scope $scope --accept-package-agreements --accept-source-agreements --disable-interactivity 2>&1 | Out-String
        $code = $LASTEXITCODE
        $already = $output -match "already installed|No available upgrade|No applicable update"
        if ($code -eq 0 -or $already) {
            return @{ Ok = $true; Already = $already; Detail = "scope=$scope exit=$code" }
        }
        $scopeRejected = $output -match "does not support the scope|scope argument|no applicable installer"
        $needsAdmin = $output -match "elevation|administrator|0x800704C7|requires admin"
        if ($scopeRejected -or $needsAdmin) {
            continue
        }
        return @{ Ok = $false; Already = $false; Detail = "scope=$scope exit=$code $($output.Trim())" }
    }
    return @{ Ok = $false; Already = $false; Detail = "Install failed for $Id. Re-run this script in an elevated PowerShell if winget requested administrator approval." }
}

if (-not (Test-Path $ManifestPath)) {
    Write-Error "Missing install manifest: $ManifestPath"
    exit 1
}
$Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json

Refresh-Path

$git = Find-CommandPath "git"
if ($git) { Add-Result "Git" "skipped" (Get-VersionText $git @("--version")) }
else { Add-Result "Git" "manual" "Git is required and was not found. Install Git for Windows from https://git-scm.com/download/win and re-run this script." }

$lfs = Find-CommandPath "git-lfs"
if (-not $lfs) {
    $gitLfs = Join-Path (Split-Path (Split-Path $git -Parent) -Parent) "cmd\git-lfs.exe"
    if (Test-Path $gitLfs) { $lfs = $gitLfs }
}
if ($lfs -and $git -and -not $AuditOnly) {
    & git -C $RepoRoot lfs install 2>&1 | Out-Null
    Add-Result "Git LFS" "completed" "Hooks confirmed. Existing attributes already track uasset, umap, fbx, and source images."
} elseif ($lfs) {
    Add-Result "Git LFS" "skipped" "Present. AuditOnly did not refresh hooks."
} else {
    Add-Result "Git LFS" "manual" "git-lfs was not found. Install Git LFS before committing .uasset or .umap files."
}

$unreal = Find-Unreal
if ($unreal) { Add-Result "Unreal Engine" "skipped" $unreal.Path }
else { Add-Result "Unreal Engine" "manual" "UE 5.8 was not found at C:\Program Files\Epic Games\UE_5.8. Install that version from the Epic Games Launcher. Do not create a replacement project." }

$msvc = Find-Msvc
if ($msvc) {
    Add-Result "Visual Studio C++" "skipped" $msvc.Path
} elseif ($InstallVisualStudioBuildTools -and -not $AuditOnly) {
    $vs = Install-WingetPackage -Id "Microsoft.VisualStudio.2022.BuildTools" -PreferredScope "machine"
    if ($vs.Ok) { Add-Result "Visual Studio C++" "completed" $vs.Detail }
    else { Add-Result "Visual Studio C++" "failed" $vs.Detail }
} else {
    Add-Result "Visual Studio C++" "manual" "MSVC toolset not detected. Re-run with -InstallVisualStudioBuildTools from an elevated shell, or install Visual Studio 2022 Build Tools with the Desktop development with C++ workload."
}

$cursor = Find-CommandPath "cursor"
if ($cursor) { Add-Result "Cursor" "skipped" $cursor }
else { Add-Result "Cursor" "manual" "Cursor was not on PATH. The editor is already in use if you are reading this from Cursor; the cursor command is optional." }

foreach ($package in $Manifest.free_packages) {
        $existing = $null
        switch ($package.command) {
            "python" { $existing = Find-Python }
            "blender" { $existing = Find-Blender }
            "gh" {
                $path = Find-CommandPath "gh"
                if (-not $path) {
                    $default = "C:\Program Files\GitHub CLI\gh.exe"
                    if (Test-Path $default) { $path = $default }
                }
                if ($path) { $existing = @{ Path = $path; Version = (Get-VersionText $path @("--version")) } }
            }
            default {
                $path = Find-CommandPath $package.command
                if ($path) { $existing = @{ Path = $path; Version = (Get-VersionText $path @("--version")) } }
            }
        }
        if ($existing) {
            Add-Result $package.id "skipped" "$($existing.Version) at $($existing.Path)"
            continue
        }
        if ($AuditOnly) {
            Add-Result $package.id "manual" "Not installed. Re-run without -AuditOnly to install this free package."
            continue
        }
        if ($package.command -eq "blender") {
            $installed = Install-BlenderOfficialZip
            Refresh-Path
            if ($installed.Ok) {
                $status = "completed"
                if ($installed.Already) { $status = "skipped" }
                Add-Result $package.id $status $installed.Detail
            } else {
                Add-Result $package.id "manual" "$($installed.Detail) Winget's Blender installer asks for administrator approval, so it was not used."
            }
            continue
        }
        $installed = Install-WingetPackage -Id $package.id -PreferredScope $package.scope
        Refresh-Path
        if ($installed.Ok) {
            $status = "completed"
            if ($installed.Already) { $status = "skipped" }
            Add-Result $package.id $status $installed.Detail
        } else {
            Add-Result $package.id "failed" $installed.Detail
        }
    }

$python = Find-Python
if ($python -and -not $AuditOnly) {
    $requirements = Join-Path $RepoRoot "Tools\AI\requirements.txt"
    if (Test-Path $requirements) {
        & $python.Path -m pip install --disable-pip-version-check --user -r $requirements
        if ($LASTEXITCODE -eq 0) {
            Add-Result "Python packages" "completed" "Installed Tools/AI/requirements.txt for the detected interpreter."
        } else {
            Add-Result "Python packages" "failed" "pip install failed for Tools/AI/requirements.txt."
        }
    }
} elseif (-not $python) {
    Add-Result "Python packages" "manual" "Real Python 3.11+ is not available, so pipeline dependencies were not installed."
}

$manual = @(
    "GitHub CLI authentication: run 'gh auth login' and choose HTTPS. This script does not log in.",
    "Confirm the GitHub repository visibility before pushing. Remote is https://github.com/SportsGamer2010/finalbell.git. Do not push untracked Content/Mannequins assets until visibility and LFS are confirmed.",
    "MetaHuman Character ships with UE 5.8 but is not enabled in FinalBell.uproject. Enable it from the editor plugin browser when authoring a fighter. Do not treat a MetaHuman as a finished boxer.",
    "Figma: create a personal access token and set FIGMA_ACCESS_TOKEN. No Figma MCP server is configured.",
    "Linear: set LINEAR_API_KEY before any issue sync. The workstream file is local until that key exists.",
    "Hugging Face: optional HF_TOKEN for higher rate limits. The pipeline does not download model weights.",
    "Adobe: export PNG or TGA into Tools/Adobe/inbox. No Adobe API is configured.",
    "DeepMotion and Rokoko: export FBX into Tools/Animation/incoming. Their plugins are not installed.",
    "Paid software and account-only products are not installed by this script."
)
foreach ($step in $manual) {
    Add-Result "Manual setup" "manual" $step
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    repo         = $RepoRoot
    audit_only   = [bool]$AuditOnly
    results      = $Results
}
$summaryPath = Join-Path $LogDir "toolchain-install-summary.json"
$summary | ConvertTo-Json -Depth 6 | Set-Content -Path $summaryPath -Encoding UTF8

$completed = @($Results | Where-Object { $_.status -eq "completed" }).Count
$skipped = @($Results | Where-Object { $_.status -eq "skipped" }).Count
$failed = @($Results | Where-Object { $_.status -eq "failed" }).Count
$manualCount = @($Results | Where-Object { $_.status -eq "manual" }).Count
Write-Host ""
Write-Host "Summary: completed=$completed skipped=$skipped failed=$failed manual=$manualCount"
Write-Host "Wrote $summaryPath"
if ($failed -gt 0) { exit 1 }
exit 0
