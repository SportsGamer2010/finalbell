import re
from dataclasses import replace

from channel_auto.models import Moment

GAME_RE = re.compile(r"\b(?:gm|game)\s*(\d+)\b", re.IGNORECASE)
OVER_RE = re.compile(r"\bover\b", re.IGNORECASE)
VS_RE = re.compile(r"\bvs\.?\b|\bversus\b", re.IGNORECASE)
MOMENT_RE = re.compile(
    r"^\s*(?:(\d+):)?(\d+):(\d+(?:\.\d+)?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*(?:\|\s*(.*?))?\s*$"
)
SPLIT_RE = re.compile(r"\s+[-|]\s+")


def _clean_name(value: str) -> str:
    return re.sub(r"\s+", " ", value).strip(" -|")


def parse_title(title: str) -> dict:
    game_number = None
    match = GAME_RE.search(title)
    if match:
        game_number = int(match.group(1))
    core = GAME_RE.sub(" ", title)
    core = re.sub(r"\s+", " ", core).strip(" -")
    teams: list[str] = []
    fighters: list[str] = []
    if OVER_RE.search(core):
        left, right = OVER_RE.split(core, maxsplit=1)
        teams = [name for name in (_clean_name(left), _clean_name(right)) if name]
    elif VS_RE.search(core):
        left, right = VS_RE.split(core, maxsplit=1)
        fighters = [name for name in (_clean_name(left), _clean_name(right)) if name]
    return {
        "title": title.strip(),
        "teams": teams,
        "fighters": fighters,
        "game_number": game_number,
    }


def _clock_to_seconds(hours: str | None, minutes: str, seconds: str) -> float:
    return int(hours or 0) * 3600 + int(minutes) * 60 + float(seconds)


def parse_roster(text: str) -> list[str]:
    names: list[str] = []
    capture = False
    for raw in text.splitlines():
        line = raw.strip()
        if line.lower() == "#roster":
            capture = True
            continue
        if capture and line.startswith("#"):
            break
        if capture and line and not line.startswith("#"):
            names.append(line)
    return names


def parse_moment_block(text: str) -> list[Moment]:
    moments: list[Moment] = []
    capture = False
    saw_header = False
    for raw in text.splitlines():
        line = raw.strip()
        if line.lower() == "#moments":
            capture = True
            saw_header = True
            continue
        if capture and line.startswith("#"):
            break
        if not capture and not saw_header:
            continue
        match = MOMENT_RE.match(line)
        if not match:
            continue
        start = _clock_to_seconds(match.group(1), match.group(2), match.group(3))
        player = _clean_name(match.group(4))
        kind = _clean_name(match.group(5)).lower() or "play"
        note = _clean_name(match.group(6) or "")
        moments.append(
            Moment(
                start=max(0.0, start - 4.0),
                end=start + 6.0,
                score=5.0,
                kind=kind,
                player=player or None,
                note=note,
            )
        )
    return moments


def split_label(label: str) -> tuple[str | None, str, str]:
    parts = [part.strip() for part in SPLIT_RE.split(label) if part.strip()]
    if len(parts) >= 2:
        return parts[0], parts[1].lower(), " ".join(parts[2:])
    return None, "play", label.strip()


def moments_from_chapters(chapters: list[dict] | None) -> list[Moment]:
    moments: list[Moment] = []
    for chapter in chapters or []:
        start = float(chapter.get("start_time") or 0)
        end = float(chapter.get("end_time") or (start + 12))
        if end <= start:
            end = start + 8
        player, kind, note = split_label(str(chapter.get("title") or "play"))
        moments.append(
            Moment(start=start, end=end, score=4.0, kind=kind, player=player, note=note)
        )
    return moments


def clamp_moment(moment: Moment, duration: float) -> Moment | None:
    if duration <= 1:
        return None
    start = min(max(0.0, moment.start), duration - 1)
    end = min(duration, max(start + 1, moment.end))
    if end - start < 3:
        return None
    return replace(moment, start=start, end=end)
