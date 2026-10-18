"""Repository paths for the automation package."""

from __future__ import annotations

from pathlib import Path

AI_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = AI_ROOT.parents[1]
CONFIG = AI_ROOT / "config"
OUTPUT = AI_ROOT / "output"
LOGS = AI_ROOT / "logs"
ADOBE_INBOX = REPO_ROOT / "Tools" / "Adobe" / "inbox"
ADOBE_OUTBOX = REPO_ROOT / "Tools" / "Adobe" / "outbox"
ANIM_INCOMING = REPO_ROOT / "Tools" / "Animation" / "incoming"
UPROJECT = REPO_ROOT / "apps" / "game-unreal" / "FinalBell.uproject"


def ensure_output(*parts: str) -> Path:
    path = OUTPUT.joinpath(*parts)
    path.mkdir(parents=True, exist_ok=True)
    return path
