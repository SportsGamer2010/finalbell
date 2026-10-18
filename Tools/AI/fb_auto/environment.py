"""Detect installed tools without installing them and without printing secrets."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from pathlib import Path
from typing import Any

from fb_auto.paths import CONFIG, REPO_ROOT, UPROJECT


def _registry_paths() -> list[str]:
    if os.name != "nt":
        return []
    import winreg

    found: list[str] = []
    queries = (
        (winreg.HKEY_CURRENT_USER, r"Environment"),
        (winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"),
    )
    for hive, subkey in queries:
        try:
            with winreg.OpenKey(hive, subkey) as key:
                value, _ = winreg.QueryValueEx(key, "Path")
        except OSError:
            continue
        found.extend(os.path.expandvars(part) for part in str(value).split(";") if part)
    return found


def which_real(name: str) -> str | None:
    folders: list[str] = []
    for folder in os.environ.get("PATH", "").split(os.pathsep) + _registry_paths():
        if folder and "WindowsApps" not in folder and folder not in folders:
            folders.append(folder)
    for folder in folders:
        for suffix in (".exe", ".cmd", ""):
            candidate = Path(folder) / f"{name}{suffix}"
            if candidate.is_file():
                return str(candidate)
    winget = Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft" / "WinGet" / "Packages"
    if winget.is_dir():
        matches = sorted(winget.glob(f"**/{name}.exe"))
        if matches:
            return str(matches[0])
    found = shutil.which(name)
    if found and "WindowsApps" not in found:
        return found
    return None


def _run_version(command: list[str]) -> str:
    try:
        completed = subprocess.run(
            command,
            check=False,
            capture_output=True,
            text=True,
            timeout=20,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return f"unavailable: {exc}"
    text = (completed.stdout or completed.stderr or "").strip().splitlines()
    return text[0] if text else f"exit {completed.returncode}"


def _python_info() -> dict[str, Any]:
    roots = [
        Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Python",
        Path(r"C:\Program Files\Python312"),
    ]
    found: list[str] = []
    for root in roots:
        if root.is_dir():
            found.extend(str(path) for path in root.glob("Python3*\\python.exe"))
    path = which_real("python")
    if path and "WindowsApps" not in path:
        found.insert(0, path)
    if not found:
        return {"installed": False, "detail": "Windows Store alias only, or Python missing"}
    version = _run_version([found[0], "--version"])
    return {"installed": version.startswith("Python 3."), "path": found[0], "version": version}


def _blender_info() -> dict[str, Any]:
    path = which_real("blender")
    roots = [
        Path(r"C:\Program Files\Blender Foundation"),
        Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "Blender Foundation",
    ]
    if not path:
        for root in roots:
            matches = sorted(root.glob("**/blender.exe")) if root.is_dir() else []
            if matches:
                path = str(matches[-1])
                break
    if not path:
        return {"installed": False, "detail": "Blender 4.2+ not found"}
    return {"installed": True, "path": path, "version": _run_version([path, "--version"])}


def _unreal_info() -> dict[str, Any]:
    editor = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
    version_file = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version")
    project = {"uproject": str(UPROJECT), "exists": UPROJECT.is_file()}
    if UPROJECT.is_file():
        project["engine_association"] = json.loads(UPROJECT.read_text(encoding="utf-8")).get(
            "EngineAssociation"
        )
    if not editor.is_file():
        return {"installed": False, "project": project, "detail": "UE 5.8 editor not found"}
    version = {}
    if version_file.is_file():
        version = json.loads(version_file.read_text(encoding="utf-8-sig"))
    return {
        "installed": True,
        "path": str(editor),
        "version": f"{version.get('MajorVersion')}.{version.get('MinorVersion')}.{version.get('PatchVersion')}",
        "project": project,
    }


def _msvc_info() -> dict[str, Any]:
    vswhere = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
    if not vswhere.is_file():
        return {"installed": False, "detail": "vswhere missing"}
    completed = subprocess.run(
        [
            str(vswhere),
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=20,
    )
    install = completed.stdout.strip()
    if not install:
        return {"installed": False, "detail": "VC toolset not installed"}
    compilers = list(Path(install).glob(r"VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe"))
    return {
        "installed": bool(compilers),
        "path": str(compilers[0]) if compilers else install,
        "version": install,
    }


def _secret_present(names: tuple[str, ...]) -> bool:
    return any(os.environ.get(name) for name in names)


def probe() -> dict[str, Any]:
    gh = which_real("gh")
    gh_auth = "not-installed"
    if gh:
        completed = subprocess.run(
            [gh, "auth", "status"],
            check=False,
            capture_output=True,
            text=True,
            timeout=20,
        )
        gh_auth = "authenticated" if completed.returncode == 0 else "not-authenticated"
    git = which_real("git")
    node = which_real("node")
    ffmpeg = which_real("ffmpeg")
    return {
        "repo": str(REPO_ROOT),
        "tools": {
            "git": {"installed": bool(git), "version": _run_version([git, "--version"]) if git else ""},
            "github_cli": {"installed": bool(gh), "auth": gh_auth, "path": gh},
            "python": _python_info(),
            "blender": _blender_info(),
            "ffmpeg": {
                "installed": bool(ffmpeg),
                "version": _run_version([ffmpeg, "-version"]) if ffmpeg else "",
            },
            "node": {"installed": bool(node), "version": _run_version([node, "--version"]) if node else ""},
            "unreal": _unreal_info(),
            "msvc": _msvc_info(),
            "cursor": {"installed": bool(which_real("cursor"))},
        },
        "integrations": {
            "figma": {
                "configured": _secret_present(("FIGMA_ACCESS_TOKEN", "FIGMA_TOKEN")),
                "mcp": False,
            },
            "linear": {"configured": _secret_present(("LINEAR_API_KEY",)), "mcp": False},
            "hugging_face": {
                "configured": _secret_present(("HF_TOKEN", "HUGGING_FACE_HUB_TOKEN")),
                "public_search_without_token": True,
                "downloads_models": False,
            },
            "adobe": {"configured": False, "handoff": "Tools/Adobe/inbox"},
            "deepmotion": {"configured": False},
            "rokoko": {"configured": False},
            "mcp_project_config": (REPO_ROOT / ".cursor" / "mcp.json").is_file(),
            "mcp_example": str(CONFIG / "mcp.example.json"),
        },
    }
