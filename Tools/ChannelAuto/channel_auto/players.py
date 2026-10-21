import json
import re
import subprocess
from dataclasses import replace
from pathlib import Path

from channel_auto.models import Moment

CARD_LABELS = {
    "PTS": "pts",
    "REB": "reb",
    "AST": "ast",
    "STL": "stl",
    "BLK": "blk",
}
SKIP_NAMES = {
    "GAMESTATS",
    "GAME",
    "STATS",
    "PTS",
    "REB",
    "AST",
    "STL",
    "BLK",
    "FG",
    "TG",
    "TO",
    "PF",
    "SF",
    "PG",
    "SG",
    "C",
    "PF",
}
STAT_KINDS = set(CARD_LABELS.values()) | {"dunk"}
PLAYER_REEL_KINDS = STAT_KINDS | {"three", "block", "steal", "ankle", "alley", "featured"}


def _center(box) -> tuple[float, float]:
    xs = [point[0] for point in box]
    ys = [point[1] for point in box]
    return sum(xs) / len(xs), sum(ys) / len(ys)


_CHROME_PIECES = (
    "SEASON",
    "EASON",
    "PERFORM",
    "GAMESTAT",
    "TOPPERF",
    "BADGE",
    "INVENTORY",
    "ARTIST",
    "TRACK",
    "NAV",
    "INVITE",
    "FAVORITE",
    "TOGGLE",
    "LOADOUT",
    "GUEST",
    "SOCIAL",
    "CONTROL",
    "PLAYGAME",
    "WORST",
    "BESTSTAT",
    "AVGTG",
)
_CHROME_EXACT = {
    "BACK",
    "SELECT",
    "APPLY",
    "UNCS",
    "NEXT",
    "SKIP",
    "DONE",
    "EXIT",
    "MENU",
    "HOME",
    "START",
    "QUIT",
    "CLOSE",
    "OPEN",
    "SAVE",
    "EDIT",
    "VIEW",
    "SORT",
    "HELP",
}


def _looks_like_chrome(folded: str) -> bool:
    upper = re.sub(r"[^A-Z0-9]", "", folded.upper())
    if not upper or len(upper) > 16 or upper in _CHROME_EXACT:
        return True
    return any(piece in upper for piece in _CHROME_PIECES)


def parse_card(items) -> tuple[str | None, dict[str, int]]:
    labels: list[tuple[str, float, float]] = []
    numbers: list[tuple[int, float, float]] = []
    names: list[tuple[str, float, float, float]] = []
    game_x = None
    for item in items or []:
        if len(item) < 3:
            continue
        box, text, confidence = item[0], str(item[1]), float(item[2])
        token = text.strip()
        if not token:
            continue
        x, y = _center(box)
        key = re.sub(r"[^A-Z]", "", token.upper())
        if "GAMESTAT" in key:
            game_x = x if game_x is None else min(game_x, x)
            continue
        if key in CARD_LABELS:
            labels.append((CARD_LABELS[key], x, y))
            continue
        if token.isdigit():
            numbers.append((int(token), x, y))
            continue
        folded = re.sub(r"[^A-Za-z0-9]", "", token)
        if len(folded) < 4 or folded.upper() in SKIP_NAMES or confidence < 0.8 or _looks_like_chrome(folded):
            continue
        names.append((folded, confidence, x, y))
    name = None
    if names:
        pool = names
        if game_x is not None:
            beside = [item for item in names if item[2] < game_x + 8]
            pool = beside or names
            name = max(pool, key=lambda item: item[2])[0]
        else:
            name = max(pool, key=lambda item: item[1])[0]
    stats: dict[str, int] = {}
    for key, label_x, label_y in labels:
        below = [number for number in numbers if abs(number[1] - label_x) < 30 and number[2] > label_y - 4]
        if not below:
            continue
        value, _, _ = min(below, key=lambda number: abs(number[1] - label_x))
        if 0 <= value <= 80:
            stats[key] = value
    return name, stats


def _edits(left: str, right: str) -> int:
    if abs(len(left) - len(right)) > 1:
        return 2
    previous = list(range(len(right) + 1))
    for i, left_char in enumerate(left, start=1):
        current = [i]
        for j, right_char in enumerate(right, start=1):
            current.append(
                min(
                    previous[j] + 1,
                    current[j - 1] + 1,
                    previous[j - 1] + (left_char != right_char),
                )
            )
        previous = current
    return previous[-1]


def _canon(name: str, aliases: dict[str, str]) -> str:
    key = re.sub(r"[^a-z0-9]", "", name.casefold())
    if not key:
        return name
    if key in aliases:
        return aliases[key]
    for other, display in list(aliases.items()):
        close = min(len(key), len(other)) >= 6 and _edits(key, other) <= 1
        nested = (
            min(len(key), len(other)) >= 5
            and abs(len(key) - len(other)) <= 2
            and (key in other or other in key)
        )
        if not close and not nested:
            continue
        chosen = name if len(re.sub(r"[^A-Za-z0-9]", "", name)) < len(re.sub(r"[^A-Za-z0-9]", "", display)) else display
        aliases[key] = chosen
        aliases[other] = chosen
        return chosen
    aliases[key] = name
    return name


def _prepare_aliases(names: list[str]) -> dict[str, str]:
    aliases: dict[str, str] = {}
    clean = [name for name in names if name and not _looks_like_chrome(name)]
    for name in clean:
        _canon(name, aliases)
    for name in clean:
        _canon(name, aliases)
    return aliases


def events_from_card(samples: list[tuple[float, str | None, dict[str, int]]], duration: float) -> list[Moment]:
    last: dict[str, dict[str, int]] = {}
    aliases = _prepare_aliases([name for _stamp, name, _stats in samples if name])
    found: list[Moment] = []
    for stamp, raw_name, stats in samples:
        if not raw_name or not stats:
            continue
        name = _canon(raw_name, aliases)
        previous = last.get(name)
        if previous is None:
            last[name] = dict(stats)
            continue
        current = dict(previous)
        for key, value in stats.items():
            old = previous.get(key)
            current[key] = old if old is not None else value
            if old is None:
                continue
            delta = value - old
            allowed = (1, 2, 3) if key == "pts" else (1,)
            if delta not in allowed:
                continue
            current[key] = value
            start = max(0.0, stamp - 6.0)
            end = min(duration, stamp + 2.0)
            note = f"+{delta} PTS" if key == "pts" else key.upper()
            weight = {3: 5, 2: 4, 1: 3}.get(delta, 3) if key == "pts" else 4 if key in {"stl", "blk"} else 2
            found.append(
                Moment(
                    start=start,
                    end=end,
                    score=float(weight),
                    kind=key,
                    player=name,
                    note=note,
                )
            )
        last[name] = current
    return _merge_same_play(found)


def unique_players(names: list[str]) -> list[str]:
    aliases: dict[str, str] = {}
    for name in names:
        if name and not _looks_like_chrome(name):
            _canon(name, aliases)
    ordered: list[str] = []
    seen: set[str] = set()
    for name in names:
        if not name or _looks_like_chrome(name):
            continue
        canon = _canon(name, aliases)
        if canon in seen:
            continue
        seen.add(canon)
        ordered.append(canon)
    return ordered


def roster_from_samples(samples: list[tuple[float, str | None, dict[str, int]]]) -> list[str]:
    aliases: dict[str, str] = {}
    counts: dict[str, int] = {}
    order: list[str] = []
    for _stamp, raw_name, _stats in samples:
        if not raw_name or _looks_like_chrome(raw_name):
            continue
        name = _canon(raw_name, aliases)
        if name not in counts:
            order.append(name)
            counts[name] = 0
        counts[name] += 1
    totals: dict[str, int] = {}
    for name, count in counts.items():
        canon = _canon(name, aliases)
        totals[canon] = totals.get(canon, 0) + count
    ordered: list[str] = []
    seen: set[str] = set()
    for name in order:
        canon = _canon(name, aliases)
        if canon in seen or totals.get(canon, 0) < 2:
            continue
        seen.add(canon)
        ordered.append(canon)
    return ordered


def presence_moments(
    samples: list[tuple[float, str | None, dict[str, int]]],
    duration: float,
    players: set[str],
) -> list[Moment]:
    if not players:
        return []
    aliases: dict[str, str] = {}
    stamps: dict[str, list[float]] = {player: [] for player in players}
    for stamp, raw_name, _stats in samples:
        if not raw_name:
            continue
        name = _canon(raw_name, aliases)
        if name in stamps:
            stamps[name].append(stamp)
    found: list[Moment] = []
    for player, times in stamps.items():
        if len(times) < 2:
            continue
        runs: list[tuple[float, float]] = []
        start = previous = times[0]
        for stamp in times[1:]:
            if stamp - previous > 2:
                runs.append((start, previous))
                start = stamp
            previous = stamp
        runs.append((start, previous))
        if len(runs) > 8:
            step = len(runs) / 8
            runs = [runs[min(len(runs) - 1, int(index * step))] for index in range(8)]
        for begin, _end in runs:
            clip_end = min(duration, begin + 6.0)
            if clip_end - begin < 4:
                continue
            found.append(
                Moment(
                    start=begin,
                    end=clip_end,
                    score=1.0,
                    kind="featured",
                    player=player,
                    note="",
                )
            )
    return found


def reels_from_card(
    samples: list[tuple[float, str | None, dict[str, int]]],
    duration: float,
    roster: list[str] | None = None,
) -> tuple[list[Moment], list[str]]:
    events = events_from_card(samples, duration)
    names = unique_players(list(roster or []) + roster_from_samples(samples))
    covered = {event.player for event in events if event.player}
    missing = {name for name in names if name not in covered}
    return events + presence_moments(samples, duration, missing), names


def _merge_same_play(moments: list[Moment]) -> list[Moment]:
    if not moments:
        return []
    ordered = sorted(moments, key=lambda moment: (moment.player or "", moment.start))
    merged: list[Moment] = []
    for moment in ordered:
        if (
            merged
            and merged[-1].player == moment.player
            and moment.start - merged[-1].start < 4
        ):
            previous = merged[-1]
            note = previous.note if moment.note in previous.note else f"{previous.note} + {moment.note}"
            merged[-1] = Moment(
                start=min(previous.start, moment.start),
                end=max(previous.end, moment.end),
                score=max(previous.score, moment.score),
                kind=previous.kind if previous.score >= moment.score else moment.kind,
                player=previous.player,
                note=note,
            )
            continue
        merged.append(moment)
    return merged


def scan_player_events(video: Path, duration: float) -> list[Moment]:
    from rapidocr_onnxruntime import RapidOCR

    work = video.parent / "work" / f"{video.stem}-cards"
    if work.exists():
        for old in work.glob("card-*.jpg"):
            old.unlink()
    work.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(video),
            "-vf",
            "fps=1,crop=620:110:640:610",
            "-q:v",
            "4",
            str(work / "card-%04d.jpg"),
        ],
        check=True,
        capture_output=True,
    )
    engine = RapidOCR()
    samples: list[tuple[float, str | None, dict[str, int]]] = []
    raw_cards: list[dict] = []
    frames = sorted(work.glob("card-*.jpg"))
    for index, frame in enumerate(frames):
        result, _ = engine(str(frame))
        name, stats = parse_card(result)
        samples.append((float(index), name, stats))
        kept = []
        for item in result or []:
            if len(item) < 3:
                continue
            x, y = _center(item[0])
            kept.append({"text": str(item[1]), "conf": float(item[2]), "x": x, "y": y})
        raw_cards.append({"t": index, "items": kept})
        if index and index % 200 == 0:
            print(f"Read {index} player cards", flush=True)
            (work / "card-read.json").write_text(json.dumps(raw_cards), encoding="utf-8")
    print(f"Read {len(frames)} player cards", flush=True)
    (work / "card-read.json").write_text(json.dumps(raw_cards), encoding="utf-8")
    events, names = reels_from_card(samples, duration)
    events = _tag_dunks(video, events, engine, work)
    for old in work.glob("*.jpg"):
        old.unlink()
    counts: dict[str, int] = {}
    for event in events:
        counts[event.player or ""] = counts.get(event.player or "", 0) + 1
    print("Roster: " + ", ".join(names))
    print("Player plays: " + ", ".join(f"{name} {count}" for name, count in sorted(counts.items())))
    return events, names


def _tag_dunks(video: Path, moments: list[Moment], engine, work: Path) -> list[Moment]:
    tagged: list[Moment] = []
    for moment in moments:
        if "PTS" not in moment.note:
            tagged.append(moment)
            continue
        stamp = max(0.0, min(moment.end - 0.4, moment.start + 6.0))
        frame = work / f"banner-{int(stamp):04d}.jpg"
        if not frame.exists():
            subprocess.run(
                [
                    "ffmpeg",
                    "-y",
                    "-hide_banner",
                    "-loglevel",
                    "error",
                    "-ss",
                    f"{stamp:.2f}",
                    "-i",
                    str(video),
                    "-vf",
                    "crop=1000:180:140:60",
                    "-frames:v",
                    "1",
                    str(frame),
                ],
                capture_output=True,
            )
        words = ""
        if frame.exists():
            result, _ = engine(str(frame))
            words = " ".join(str(item[1]).upper() for item in (result or []) if len(item) > 1)
        if "+2 PTS" in moment.note and ("DUNK" in words or "POSTER" in words):
            note = moment.note if "DUNK" in moment.note else f"{moment.note} DUNK"
            tagged.append(replace(moment, kind="dunk", note=note, score=max(moment.score, 6)))
            continue
        tagged.append(moment)
    return tagged
