import array
import subprocess
import wave
from pathlib import Path

from channel_auto.models import Moment


def extract_wav(video: Path, wav: Path) -> None:
    wav.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(video),
            "-ac",
            "1",
            "-ar",
            "16000",
            "-vn",
            str(wav),
        ],
        check=True,
        capture_output=True,
    )


def rms_windows(wav_path: Path, hop_sec: float = 0.5) -> list[float]:
    with wave.open(str(wav_path), "rb") as handle:
        if handle.getsampwidth() != 2 or handle.getnchannels() != 1:
            raise RuntimeError("expected mono 16-bit audio")
        hop = max(1, int(handle.getframerate() * hop_sec))
        values: list[float] = []
        while True:
            raw = handle.readframes(hop)
            usable = raw[: len(raw) // 2 * 2]
            if len(usable) < 2:
                break
            data = array.array("h")
            data.frombytes(usable)
            total = 0
            for sample in data:
                total += sample * sample
            values.append((total / len(data)) ** 0.5)
        return values


def select_peaks(
    levels: list[float],
    *,
    hop_sec: float,
    duration: float,
    min_gap: float,
    pad_before: float,
    pad_after: float,
    top_n: int,
    kind: str,
) -> list[Moment]:
    if not levels or duration <= 0:
        return []
    mean = sum(levels) / len(levels)
    variance = sum((value - mean) ** 2 for value in levels) / len(levels)
    spread = variance**0.5 or 1.0
    candidates: list[tuple[float, float]] = []
    for index, value in enumerate(levels):
        if index > 0 and value < levels[index - 1]:
            continue
        if index < len(levels) - 1 and value < levels[index + 1]:
            continue
        score = (value - mean) / spread
        if score < 1.15:
            continue
        candidates.append((score, index * hop_sec))
    candidates.sort(reverse=True)
    chosen: list[tuple[float, float]] = []
    for score, stamp in candidates:
        if any(abs(stamp - kept) < min_gap for _, kept in chosen):
            continue
        chosen.append((score, stamp))
        if len(chosen) >= top_n:
            break
    moments: list[Moment] = []
    for score, stamp in sorted(chosen, key=lambda item: item[1]):
        start = max(0.0, stamp - pad_before)
        end = min(duration, stamp + pad_after)
        if end - start < 4:
            continue
        moments.append(Moment(start=start, end=end, score=score, kind=kind))
    return moments


def knockout_windows(hit_seconds: list[int], duration: float) -> list[Moment]:
    if not hit_seconds or duration <= 0:
        return []
    ordered = sorted(set(hit_seconds))
    clusters: list[list[int]] = [[ordered[0]]]
    for stamp in ordered[1:]:
        if stamp - clusters[-1][-1] <= 3:
            clusters[-1].append(stamp)
        else:
            clusters.append([stamp])
    moments: list[Moment] = []
    for cluster in clusters:
        begin = float(cluster[0])
        start = max(0.0, begin - 14.0)
        end = min(duration, begin + 8.0)
        if end - start < 6:
            continue
        moments.append(Moment(start=start, end=end, score=5.0, kind="ko"))
    return moments


def find_knockouts(video: Path, duration: float) -> list[Moment]:
    width, height = 96, 54
    process = subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(video),
            "-vf",
            f"fps=1,scale={width}:{height}",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "rgb24",
            "-",
        ],
        capture_output=True,
        check=True,
    )
    frame_size = width * height * 3
    data = process.stdout
    hits: list[int] = []
    pixels = width * height
    for index in range(len(data) // frame_size):
        chunk = data[index * frame_size : (index + 1) * frame_size]
        white = 0
        blue = 0
        for offset in range(0, frame_size, 3):
            red, green, blue_channel = chunk[offset], chunk[offset + 1], chunk[offset + 2]
            if red > 210 and green > 210 and blue_channel > 210:
                white += 1
            if blue_channel > red + 35 and blue_channel > 90 and red < 140:
                blue += 1
        if white / pixels >= 0.04 and blue / pixels >= 0.40:
            hits.append(index)
    return knockout_windows(hits, duration)


def even_windows(duration: float, count: int, length: float) -> list[Moment]:
    if duration <= 0:
        return []
    if duration <= length:
        return [Moment(0.0, duration, score=0.0, kind="play")]
    span = duration - length
    steps = max(1, count - 1)
    return [
        Moment(span * index / steps, span * index / steps + length, score=0.0, kind="play")
        for index in range(count)
    ]
