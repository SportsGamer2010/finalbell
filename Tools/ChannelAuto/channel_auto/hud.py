import re
import subprocess
from pathlib import Path

STOPWORDS = {
    "pts",
    "reb",
    "ast",
    "stl",
    "blk",
    "game",
    "stats",
    "chemistry",
    "takeover",
    "over",
    "under",
    "ppg",
    "rpg",
    "apg",
    "spg",
    "bpg",
    "fgm",
    "fga",
    "quarter",
    "shot",
    "clock",
    "bonus",
    "foul",
    "pro",
    "nba",
}

TAG_RE = re.compile(r"[A-Za-z][A-Za-z0-9_.-]{3,20}")
_ENGINE = None


def fold(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def match_roster(text: str | None, roster: list[str]) -> str | None:
    if not text:
        return None
    folded = fold(text)
    if not folded:
        return None
    for name in roster:
        token = fold(name)
        if token and token in folded:
            return name
    return None


def clean_gamertag(text: str | None) -> str | None:
    if not text:
        return None
    tokens = [
        token
        for token in TAG_RE.findall(text)
        if token.lower() not in STOPWORDS
    ]
    if not tokens:
        return None
    return max(tokens, key=len)


def resolve_name(text: str | None, roster: list[str]) -> str | None:
    matched = match_roster(text, roster)
    if matched:
        return matched
    if roster:
        return None
    return clean_gamertag(text)


def ocr_available() -> bool:
    try:
        import rapidocr_onnxruntime  # noqa: F401
    except ImportError:
        return False
    return True


def ocr_image(path: Path) -> str:
    global _ENGINE
    try:
        from rapidocr_onnxruntime import RapidOCR
    except ImportError:
        return ""
    if _ENGINE is None:
        _ENGINE = RapidOCR()
    result, _ = _ENGINE(str(path))
    if not result:
        return ""
    return " ".join(str(item[1]) for item in result if len(item) > 1)


def _crop(frame: Path, dest: Path, box: str) -> None:
    subprocess.run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(frame),
            "-vf",
            box,
            "-update",
            "1",
            str(dest),
        ],
        check=True,
        capture_output=True,
    )


def extract_frame(video: Path, stamp: float, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-ss",
            f"{stamp:.3f}",
            "-i",
            str(video),
            "-frames:v",
            "1",
            "-update",
            "1",
            str(dest),
        ],
        check=True,
        capture_output=True,
    )


def read_player_name(frame: Path, roster: list[str], work: Path) -> str | None:
    if not ocr_available():
        return None
    crops = {
        "bug": "crop=iw*0.36:ih*0.09:iw*0.38:ih*0.86",
        "tag": "crop=iw*0.42:ih*0.14:iw*0.08:ih*0.58",
    }
    texts = []
    for name, box in crops.items():
        dest = work / f"{frame.stem}_{name}.jpg"
        _crop(frame, dest, box)
        texts.append(ocr_image(dest))
    for text in texts:
        matched = match_roster(text, roster)
        if matched:
            return matched
    if roster:
        return None
    for text in reversed(texts):
        tag = clean_gamertag(text)
        if tag:
            return tag
    return None
