import json
import os
import time
from pathlib import Path


def load_state(path: Path) -> dict:
    if not path.exists():
        return {"baselined": False, "videos": {}}
    payload = json.loads(path.read_text(encoding="utf-8"))
    payload.setdefault("baselined", False)
    payload.setdefault("videos", {})
    return payload


def save_state(path: Path, state: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(state, indent=2), encoding="utf-8")


def knockout_render_current(record: dict | None) -> bool:
    if not record or record.get("status") not in {"rendered", "uploaded"}:
        return False
    return any("_knockouts" in Path(path).name for path in record.get("outputs") or [])


def should_process(record: dict | None, *, backfill: bool, force: bool) -> bool:
    if force or record is None:
        return True
    status = record.get("status")
    if status == "baseline":
        return backfill
    if status in {"rendered", "uploaded", "failed"}:
        return False
    if status == "error" and int(record.get("attempts", 0)) >= 3:
        return False
    return True


def pid_running(pid: int) -> bool:
    if pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def acquire_lock(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        try:
            owner = int(path.read_text(encoding="utf-8").strip() or "0")
        except ValueError:
            owner = 0
        age = time.time() - path.stat().st_mtime
        if pid_running(owner) and age < 6 * 3600 and owner != os.getpid():
            raise SystemExit(f"Another watcher is already running (pid {owner}).")
    path.write_text(str(os.getpid()), encoding="utf-8")


def touch_lock(path: Path) -> None:
    if path.exists():
        path.write_text(str(os.getpid()), encoding="utf-8")


def release_lock(path: Path) -> None:
    if not path.exists():
        return
    try:
        owner = int(path.read_text(encoding="utf-8").strip() or "0")
    except ValueError:
        owner = 0
    if owner == os.getpid():
        path.unlink(missing_ok=True)
