import subprocess
from pathlib import Path

from channel_auto.models import Moment

# Opponent score is the black pair on the right orange pill of a 1280x720 Crews scoreboard.
# UNCS, the coached team, is the left pill. Digits are 8x12 bit rows.
OPPONENT_BOX = (458, 650, 525, 684)
_TEMPLATES: dict[str, list[tuple[int, ...]]] = {
    "0": [
        (60, 126, 231, 195, 195, 195, 195, 195, 195, 231, 126, 60),
    ],
    "1": [
        (240, 255, 255, 240, 240, 240, 240, 240, 240, 240, 240, 240),
    ],
    "7": [
        (255, 255, 224, 112, 48, 56, 24, 24, 28, 12, 12, 12),
    ],
    "9": [
        (60, 126, 231, 195, 195, 231, 254, 200, 224, 103, 126, 28),
    ],
    "3": [
        (60, 127, 227, 192, 224, 120, 120, 192, 195, 195, 127, 60),
        (124, 254, 195, 192, 224, 120, 248, 192, 195, 195, 254, 60),
    ],
    "8": [
        (60, 254, 195, 195, 199, 126, 255, 195, 195, 195, 255, 60),
        (60, 127, 227, 195, 99, 126, 127, 195, 195, 227, 127, 60),
    ],
    "2": [
        (62, 127, 227, 195, 192, 96, 112, 56, 14, 7, 255, 255),
        (60, 127, 227, 195, 192, 96, 112, 56, 14, 7, 255, 255),
        (124, 254, 199, 195, 192, 192, 96, 56, 28, 14, 255, 255),
        (60, 126, 199, 195, 192, 224, 112, 56, 28, 7, 255, 255),
    ],
    "4": [
        (112, 112, 120, 104, 100, 102, 99, 255, 255, 96, 96, 96),
    ],
    "5": [
        (254, 254, 3, 3, 127, 255, 194, 192, 195, 227, 127, 60),
    ],
    "6": [
        (60, 126, 195, 3, 59, 127, 195, 195, 195, 227, 126, 60),
        (124, 126, 199, 3, 123, 127, 195, 195, 195, 231, 126, 60),
    ],
}


def _bits(row: int) -> int:
    return row.bit_count()


def _distance(left: tuple[int, ...], right: tuple[int, ...]) -> int:
    return sum(_bits(a ^ b) for a, b in zip(left, right))


def _pixel(frame: bytes, width: int, x: int, y: int) -> tuple[int, int, int]:
    index = (y * width + x) * 3
    return frame[index], frame[index + 1], frame[index + 2]


def _bitmap(frame: bytes, width: int, box: tuple[int, int, int, int], dark) -> list[tuple[int, ...]]:
    x0, y0, x1, y1 = box
    height = y1 - y0
    span = x1 - x0
    mask = []
    columns = [0] * span
    for y in range(height):
        row = []
        for x in range(span):
            hit = dark(_pixel(frame, width, x0 + x, y0 + y))
            row.append(hit)
            if hit:
                columns[x] += 1
        mask.append(row)
    spans: list[tuple[int, int]] = []
    start = None
    for index, count in enumerate(columns):
        if count >= 3 and start is None:
            start = index
        elif count < 3 and start is not None:
            if index - start >= 3:
                spans.append((start, index))
            start = None
    if start is not None and span - start >= 3:
        spans.append((start, span))
    digits: list[tuple[int, ...]] = []
    for left, right in spans[:2]:
        ys = [y for y in range(height) if any(mask[y][x] for x in range(left, right))]
        if not ys:
            continue
        top, bottom = min(ys), max(ys) + 1
        grid: list[int] = []
        for gy in range(12):
            bits = 0
            for gx in range(8):
                x = min(span - 1, left + int((gx + 0.5) * (right - left) / 8))
                y = min(height - 1, top + int((gy + 0.5) * (bottom - top) / 12))
                if mask[y][x]:
                    bits |= 1 << gx
            grid.append(bits)
        digits.append(tuple(grid))
    return digits


def _black(rgb: tuple[int, int, int]) -> bool:
    return sum(rgb) < 100


def _match(bitmap: tuple[int, ...]) -> str | None:
    ranked: list[tuple[int, str]] = []
    for digit, samples in _TEMPLATES.items():
        ranked.append((min(_distance(bitmap, sample) for sample in samples), digit))
    ranked.sort()
    best_distance, best_digit = ranked[0]
    if best_distance > 16:
        return None
    if len(ranked) > 1 and ranked[1][0] - best_distance < 6:
        return None
    return best_digit


def read_opponent_score(frame: bytes, width: int, height: int) -> int | None:
    if width < 200 or height < 100:
        return None
    scale_x = width / 1280
    scale_y = height / 720
    box = tuple(int(value * (scale_x if index % 2 == 0 else scale_y)) for index, value in enumerate(OPPONENT_BOX))
    digits = _bitmap(frame, width, box, _black)
    if not digits:
        return None
    chars = []
    for bitmap in digits:
        digit = _match(bitmap)
        if digit is None:
            return None
        chars.append(digit)
    return int("".join(chars))


def clean_scores(samples: list[tuple[float, int | None]]) -> list[tuple[float, int]]:
    cleaned: list[tuple[float, int]] = []
    last = None
    hold = None
    hold_count = 0
    for stamp, score in samples:
        if score is None or score < 0 or score > 150:
            hold = None
            hold_count = 0
            continue
        if score == hold:
            hold_count += 1
        else:
            hold = score
            hold_count = 1
        if hold_count < 2 or score == last:
            continue
        if last is None:
            if score <= 12:
                last = score
                cleaned.append((stamp, score))
            continue
        if score - last in (1, 2, 3):
            last = score
            cleaned.append((stamp, score))
    return cleaned


def scoring_events(samples: list[tuple[float, int | None]], duration: float) -> list[Moment]:
    events: list[Moment] = []
    previous = None
    for stamp, score in clean_scores(samples):
        if previous is None:
            previous = score
            continue
        delta = score - previous
        if delta in (1, 2, 3):
            start = max(0.0, stamp - 10.0)
            end = min(duration, stamp + 2.0)
            events.append(
                Moment(
                    start=start,
                    end=end,
                    score=float(delta),
                    kind="allowed",
                    note=f"+{delta} opponent {score}",
                )
            )
        previous = score
    return events


def _opponent_total(event: Moment) -> int:
    parts = event.note.split()
    if len(parts) >= 3 and parts[-2] == "opponent":
        return int(parts[-1])
    return 0


def select_coach_film(events: list[Moment], limit: float = 72.0) -> list[Moment]:
    if not events:
        return []
    late = [event for event in events if _opponent_total(event) >= 40]
    pool = late or events
    chosen: list[Moment] = []
    total = 0.0
    for event in reversed(pool):
        duration = event.end - event.start
        if chosen and total + duration > limit:
            break
        chosen.append(event)
        total += duration
    opening = [event for event in events if _opponent_total(event) <= 11 and event.score >= 3]
    if opening and total + (opening[0].end - opening[0].start) <= limit + 4:
        chosen.append(opening[0])
    return sorted(chosen, key=lambda event: event.start)


def scan_opponent_scores(video: Path, unknowns: list | None = None) -> list[tuple[float, int | None]]:
    width, height = 1280, 80
    origin_y = 640
    process = subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(video),
            "-vf",
            f"fps=1,crop={width}:{height}:0:{origin_y}",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "rgb24",
            "-",
        ],
        check=True,
        capture_output=True,
    )
    frame_size = width * height * 3
    data = process.stdout
    samples: list[tuple[float, int | None]] = []
    for index in range(len(data) // frame_size):
        crop = data[index * frame_size : (index + 1) * frame_size]
        # Lift the crop back into a full-height buffer so the score box stays put.
        frame = bytearray(width * 720 * 3)
        start = origin_y * width * 3
        frame[start : start + frame_size] = crop
        raw = bytes(frame)
        score = read_opponent_score(raw, width, 720)
        if score is None and unknowns is not None and index > 350 and len(unknowns) < 40:
            digits = _bitmap(raw, width, OPPONENT_BOX, _black)
            if digits and any(_match(item) is None for item in digits):
                unknowns.append((index, digits))
        samples.append((float(index), score))
    return samples
