"""Write guards. Generated files stay outside the Unreal source and content trees."""

from __future__ import annotations

from pathlib import Path

from fb_auto.paths import REPO_ROOT

PROTECTED = (
    REPO_ROOT / "apps" / "game-unreal" / "Source",
    REPO_ROOT / "apps" / "game-unreal" / "Content",
    REPO_ROOT / "apps" / "game-unreal" / "Config",
)

ALLOWED_PREFIXES = (
    REPO_ROOT / "Tools" / "AI" / "output",
    REPO_ROOT / "Tools" / "AI" / "logs",
    REPO_ROOT / "Tools" / "Adobe" / "outbox",
    REPO_ROOT / "Tools" / "Animation" / "manifests",
)


def assert_safe_output(path: Path) -> Path:
    resolved = path.resolve()
    if any(_is_relative_to(resolved, protected) for protected in PROTECTED):
        raise RuntimeError(f"Refusing to write into protected project path: {resolved}")
    if not any(_is_relative_to(resolved, allowed) for allowed in ALLOWED_PREFIXES):
        raise RuntimeError(f"Refusing to write outside the toolchain output folders: {resolved}")
    resolved.parent.mkdir(parents=True, exist_ok=True)
    return resolved


def _is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent.resolve())
        return True
    except ValueError:
        return False
