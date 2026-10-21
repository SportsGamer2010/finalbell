import subprocess
from pathlib import Path

from channel_auto.config import ROOT
from channel_auto.models import Moment

FONTS = "C:/Windows/Fonts"
THUMB_FONT = "arialbd.ttf"
LEAGUE_LOGO = ROOT / "brand" / "2kcomp-league.png"
BOXING_LOGO = ROOT / "brand" / "esbl-logo.png"
LEAGUE_NAME = "2K COMP LEAGUE"
LEAGUE_URL = "2KCOMPLEAGUE.COM"
BOXING_NAME = "ESPORTS BOXING LEAGUE"
BOXING_URL = "ESPORTSBOXINGLEAGUE.COM"
GAME_BED = (
    "highpass=f=70,"
    "pan=stereo|c0=c0-0.65*c1|c1=c1-0.65*c0,"
    "volume=1.7,"
    "acompressor=threshold=-20dB:ratio=2.5:attack=20:release=200"
)


def _run(command: list[str], cwd: Path | None = None) -> None:
    result = subprocess.run(command, cwd=cwd, capture_output=True)
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace")[-2000:]
        raise RuntimeError(detail or f"Command exited {result.returncode}")


def probe_duration(path: Path) -> float:
    result = subprocess.run(
        [
            "ffprobe",
            "-v",
            "error",
            "-show_entries",
            "format=duration",
            "-of",
            "default=noprint_wrappers=1:nokey=1",
            str(path),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return float(result.stdout.strip())


def probe_size(path: Path) -> tuple[int, int]:
    result = subprocess.run(
        [
            "ffprobe",
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=width,height",
            "-of",
            "csv=s=x:p=0",
            str(path),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    width, height = result.stdout.strip().split("x")
    return int(width), int(height)


def _ass_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace("{", "\\{").replace("}", "\\}").replace("\n", "\\N")


def _clock(seconds: float) -> str:
    seconds = max(0.0, seconds)
    hours = int(seconds // 3600)
    minutes = int((seconds % 3600) // 60)
    remain = seconds % 60
    return f"{hours}:{minutes:02d}:{remain:05.2f}"


def write_ass(path: Path, hook: str, lines: list[str], duration: float) -> None:
    events = [
        "Dialogue: 0,"
        + _clock(0)
        + ","
        + _clock(min(2.6, duration))
        + ",Hook,,0,0,0,,"
        + _ass_escape(hook.rstrip("."))
    ]
    cues = [line.strip().rstrip(".") for line in lines if line.strip()] or [hook]
    slot = max(1.2, duration / len(cues))
    for index, line in enumerate(cues):
        start = min(duration, index * slot)
        end = duration if index == len(cues) - 1 else min(duration, (index + 1) * slot)
        events.append(
            "Dialogue: 0,"
            + _clock(start)
            + ","
            + _clock(end)
            + ",Cap,,0,0,0,,"
            + _ass_escape(line)
        )
    body = "\n".join(
        [
            "[Script Info]",
            "ScriptType: v4.00+",
            "PlayResX: 1080",
            "PlayResY: 1920",
            "WrapStyle: 0",
            "",
            "[V4+ Styles]",
            "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding",
            "Style: Hook,Arial,68,&H00FFFFFF,&H000000FF,&H00000000,&HA0000000,-1,0,0,0,100,100,0,0,1,4,0,8,70,70,180,1",
            "Style: Cap,Arial,52,&H00FFFFFF,&H000000FF,&H00000000,&HA0000000,-1,0,0,0,100,100,0,0,1,4,0,2,80,80,260,1",
            "",
            "[Events]",
            "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text",
            *events,
            "",
        ]
    )
    path.write_text(body, encoding="utf-8")


def _cut(source: Path, moment: Moment, dest: Path, *, duck_voice: bool = False) -> None:
    command = [
        "ffmpeg",
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-ss",
        f"{moment.start:.3f}",
        "-to",
        f"{moment.end:.3f}",
        "-i",
        str(source),
    ]
    if duck_voice:
        command.extend(["-af", GAME_BED])
    command.extend(
        [
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "20",
            "-c:a",
            "aac",
            "-ar",
            "48000",
            "-ac",
            "2",
            "-movflags",
            "+faststart",
            str(dest),
        ]
    )
    _run(command)


def _concat(parts: list[Path], dest: Path, work: Path) -> None:
    if len(parts) == 1:
        dest.write_bytes(parts[0].read_bytes())
        return
    listing = work / "list.txt"
    lines = []
    for part in parts:
        escaped = part.resolve().as_posix().replace("'", "'\\''")
        lines.append(f"file '{escaped}'")
    listing.write_text("\n".join(lines) + "\n", encoding="utf-8")
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-f",
            "concat",
            "-safe",
            "0",
            "-i",
            str(listing),
            "-c",
            "copy",
            str(dest),
        ]
    )


def _filter_video(use_captions: bool) -> str:
    scale = "scale=1080:1920:force_original_aspect_ratio=increase,crop=1080:1920"
    if not use_captions:
        return scale
    fonts = FONTS.replace(":", "\\:")
    return f"{scale},subtitles=cap.ass:fontsdir={fonts}"


def _finish(joined: Path, dest: Path, work: Path, voice: Path | None, use_captions: bool) -> None:
    video_filter = _filter_video(use_captions)
    command = ["ffmpeg", "-y", "-hide_banner", "-loglevel", "error", "-i", str(joined)]
    if voice is not None:
        command.extend(["-i", str(voice)])
        filter_complex = (
            f"[0:v]{video_filter}[v];"
            "[0:a]volume=0.85,aresample=48000[g];"
            "[1:a]volume=1.4,aresample=48000[s];"
            "[g][s]amix=inputs=2:duration=first:normalize=0[a]"
        )
        command.extend(
            [
                "-filter_complex",
                filter_complex,
                "-map",
                "[v]",
                "-map",
                "[a]",
            ]
        )
    else:
        command.extend(["-vf", video_filter])
    command.extend(
        [
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "20",
            "-c:a",
            "aac",
            "-b:a",
            "160k",
            "-movflags",
            "+faststart",
            str(dest),
        ]
    )
    _run(command, cwd=work)


def thumbnail_lines(hook: str) -> tuple[str, str]:
    words = [word for word in hook.replace("#shorts", "").replace(".", " ").split() if word]
    if not words:
        return "WATCH THIS", ""
    lines: list[str] = []
    current = ""
    for word in words:
        trial = f"{current} {word}".strip()
        if current and len(trial) > 18:
            lines.append(current)
            current = word
            if len(lines) == 2:
                break
        else:
            current = trial
    if len(lines) < 2 and current:
        lines.append(current)
    while len(lines) < 2:
        lines.append("")
    return lines[0].upper(), lines[1].upper()


def _drawtext(text: str) -> str:
    escaped = (
        text.replace("\\", "\\\\")
        .replace(":", "\\:")
        .replace("'", "\\'")
        .replace("%", "\\%")
        .replace(",", "\\,")
    )
    return escaped


def write_thumbnail(source: Path, stamp: float, hook: str, badge: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    line_a, line_b = thumbnail_lines(hook)
    filters = [
        "scale=1920:1080:force_original_aspect_ratio=increase",
        "crop=1920:1080",
        "eq=contrast=1.16:saturation=1.35:brightness=0.03",
        "unsharp=5:5:0.6:5:5:0.0",
        "drawbox=x=0:y=0:w=1920:h=150:color=black@0.5:t=fill",
        "drawbox=x=0:y=690:w=1920:h=390:color=black@1.0:t=fill",
        "drawbox=x=0:y=690:w=16:h=390:color=0xFFE14A:t=fill",
        "drawbox=x=64:y=52:w=18:h=18:color=0xFFE14A:t=fill",
        (
            f"drawtext=fontfile={THUMB_FONT}:text='{_drawtext(badge.upper())}'"
            ":fontsize=46:fontcolor=0xFFE14A:x=100:y=40"
        ),
        (
            f"drawtext=fontfile={THUMB_FONT}:text='{_drawtext(line_a)}'"
            ":fontsize=84:fontcolor=white:borderw=8:bordercolor=black:x=64:y=760"
        ),
    ]
    if line_b:
        filters.append(
            f"drawtext=fontfile={THUMB_FONT}:text='{_drawtext(line_b)}'"
            ":fontsize=84:fontcolor=0xFFE14A:borderw=8:bordercolor=black:x=64:y=870"
        )
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-ss",
            f"{max(0.0, stamp):.3f}",
            "-i",
            str(source),
            "-frames:v",
            "1",
            "-vf",
            ",".join(filters),
            "-q:v",
            "2",
            "-update",
            "1",
            str(dest),
        ],
        cwd=Path(FONTS),
    )
    if dest.stat().st_size > 2_000_000:
        _run(
            [
                "ffmpeg",
                "-y",
                "-hide_banner",
                "-loglevel",
                "error",
                "-i",
                str(dest),
                "-q:v",
                "6",
                "-update",
                "1",
                str(dest),
            ]
        )


def write_coach_still(source: Path, stamp: float, label: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    filters = [
        "scale=1920:1080:force_original_aspect_ratio=increase",
        "crop=1920:1080",
        "drawbox=x=0:y=0:w=1920:h=88:color=black@0.78:t=fill",
        (
            f"drawtext=fontfile={THUMB_FONT}:text='{_drawtext(label[:48])}'"
            ":fontsize=40:fontcolor=white:x=40:y=22"
        ),
    ]
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-ss",
            f"{max(0.0, stamp):.3f}",
            "-i",
            str(source),
            "-frames:v",
            "1",
            "-vf",
            ",".join(filters),
            "-q:v",
            "2",
            "-update",
            "1",
            str(dest),
        ],
        cwd=Path(FONTS),
    )


def render_clip(
    source: Path,
    moments: list[Moment],
    hook: str,
    lines: list[str],
    dest: Path,
    voice: Path | None = None,
    work_dir: Path | None = None,
    *,
    duck_voice: bool = False,
) -> None:
    if not moments:
        raise ValueError("A clip needs at least one moment.")
    dest.parent.mkdir(parents=True, exist_ok=True)
    work = work_dir or (dest.parent / "_work" / dest.stem)
    work.mkdir(parents=True, exist_ok=True)
    parts: list[Path] = []
    for index, moment in enumerate(moments):
        part = work / f"part_{index:02d}.mp4"
        _cut(source, moment, part, duck_voice=duck_voice)
        parts.append(part)
    joined = work / "joined.mp4"
    _concat(parts, joined, work)
    duration = probe_duration(joined)
    write_ass(work / "cap.ass", hook, lines, duration)
    voice_file = voice if voice and voice.exists() else None
    try:
        _finish(joined, dest, work, voice_file, True)
    except RuntimeError:
        _finish(joined, dest, work, voice_file, False)
        (work / "caption-fallback.txt").write_text(hook, encoding="utf-8")


def _logo_ready(logo: Path) -> Path:
    if not logo.exists():
        raise FileNotFoundError(f"Logo is missing: {logo}")
    return logo


def brand_short(
    source: Path,
    dest: Path,
    *,
    logo: Path,
    name: str,
    url: str,
    url_color: str,
) -> None:
    mark = _logo_ready(logo)
    dest.parent.mkdir(parents=True, exist_ok=True)
    out = dest
    temporary = None
    if source.resolve() == dest.resolve():
        temporary = dest.with_suffix(".branding.mp4")
        out = temporary
    filter_complex = (
        "[1:v]colorkey=0x000000:0.16:0.08,scale=210:-1[lg];"
        "[0:v]drawbox=x=28:y=36:w=700:h=128:color=black@0.72:t=fill[plate];"
        "[plate][lg]overlay=44:48[withlogo];"
        f"[withlogo]drawtext=fontfile=arialbd.ttf:text='{name}'"
        ":fontsize=28:fontcolor=white:x=250:y=58,"
        f"drawtext=fontfile=arialbd.ttf:text='{url}'"
        f":fontsize=24:fontcolor={url_color}:x=250:y=108[v]"
    )
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(source),
            "-i",
            str(mark),
            "-filter_complex",
            filter_complex,
            "-map",
            "[v]",
            "-map",
            "0:a",
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "18",
            "-c:a",
            "aac",
            "-b:a",
            "160k",
            "-movflags",
            "+faststart",
            str(out),
        ],
        cwd=Path(FONTS),
    )
    if temporary is not None:
        temporary.replace(dest)


def brand_promo(source: Path, dest: Path, headline: str, kicker: str, logo: Path | None = None) -> None:
    mark = _logo_ready(logo or LEAGUE_LOGO)
    dest.parent.mkdir(parents=True, exist_ok=True)
    out = dest
    temporary = None
    if source.resolve() == dest.resolve():
        temporary = dest.with_suffix(".branding.mp4")
        out = temporary
    duration = probe_duration(source)
    end = max(0.0, duration - 2.3)
    plate = (
        "[1:v]colorkey=0x000000:0.16:0.08,scale=210:-1[lg];"
        "[0:v]drawbox=x=28:y=36:w=700:h=128:color=black@0.72:t=fill[plate];"
        "[plate][lg]overlay=44:48[withlogo];"
        "[withlogo]drawtext=fontfile=arialbd.ttf:text='2K COMP LEAGUE'"
        ":fontsize=28:fontcolor=white:x=250:y=58,"
        "drawtext=fontfile=arialbd.ttf:text='2KCOMPLEAGUE.COM'"
        ":fontsize=24:fontcolor=0x4DA3FF:x=250:y=108,"
        "drawbox=x=0:y=210:w=1080:h=150:color=black@0.62:t=fill:enable='lt(t\\,2.6)',"
        f"drawtext=fontfile=arialbd.ttf:text='{_drawtext(headline.upper())}'"
        ":fontsize=68:fontcolor=white:borderw=6:bordercolor=black"
        ":x=48:y=248:enable='lt(t\\,2.6)',"
        "drawbox=x=0:y=1620:w=1080:h=300:color=black@0.88:t=fill"
        f":enable='gte(t\\,{end:.2f})',"
        f"drawtext=fontfile=arialbd.ttf:text='{_drawtext(kicker.upper())}'"
        ":fontsize=52:fontcolor=0xFFE14A:x=48:y=1700"
        f":enable='gte(t\\,{end:.2f})',"
        "drawtext=fontfile=arialbd.ttf:text='2KCOMPLEAGUE.COM'"
        ":fontsize=40:fontcolor=white:x=48:y=1788"
        f":enable='gte(t\\,{end:.2f})'[v]"
    )
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(source),
            "-i",
            str(mark),
            "-filter_complex",
            plate,
            "-map",
            "[v]",
            "-map",
            "0:a",
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "18",
            "-c:a",
            "aac",
            "-b:a",
            "160k",
            "-movflags",
            "+faststart",
            str(out),
        ],
        cwd=Path(FONTS),
    )
    if temporary is not None:
        temporary.replace(dest)


def brand_player(source: Path, dest: Path, name: str, kicker: str, logo: Path | None = None) -> None:
    mark = _logo_ready(logo or LEAGUE_LOGO)
    dest.parent.mkdir(parents=True, exist_ok=True)
    out = dest
    temporary = None
    if source.resolve() == dest.resolve():
        temporary = dest.with_suffix(".branding.mp4")
        out = temporary
    shown = _drawtext(name.upper()[:24])
    chip = _drawtext((kicker or "2K COMP LEAGUE").upper()[:48])
    plate = (
        "[1:v]colorkey=0x000000:0.16:0.08,scale=148:-1[lg];"
        "[0:v][lg]overlay=36:36[marked];"
        "[marked]drawbox=x=0:y=1688:w=1080:h=232:color=black@0.92:t=fill,"
        "drawbox=x=0:y=1688:w=1080:h=8:color=0xFFE14A@1.0:t=fill,"
        f"drawtext=fontfile=arialbd.ttf:text='{shown}'"
        ":fontsize=58:fontcolor=white:x=48:y=1724,"
        f"drawtext=fontfile=arialbd.ttf:text='{chip}'"
        ":fontsize=30:fontcolor=0xFFE14A:x=48:y=1804,"
        "drawtext=fontfile=arialbd.ttf:text='2KCOMPLEAGUE.COM'"
        ":fontsize=26:fontcolor=0x4DA3FF:x=48:y=1860[v]"
    )
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(source),
            "-i",
            str(mark),
            "-filter_complex",
            plate,
            "-map",
            "[v]",
            "-map",
            "0:a",
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "18",
            "-c:a",
            "aac",
            "-b:a",
            "160k",
            "-movflags",
            "+faststart",
            str(out),
        ],
        cwd=Path(FONTS),
    )
    if temporary is not None:
        temporary.replace(dest)


def write_roster_card(source: Path, dest: Path, name: str, logo: Path | None = None) -> None:
    mark = _logo_ready(logo or LEAGUE_LOGO)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shown = _drawtext(name.upper()[:24])
    plate = (
        "[2:v]colorkey=0x000000:0.16:0.08,scale=180:-1[lg];"
        "[0:v][lg]overlay=(W-w)/2:620[marked];"
        f"[marked]drawtext=fontfile=arialbd.ttf:text='{shown}'"
        ":fontsize=64:fontcolor=white:x=48:y=860,"
        "drawbox=x=48:y=960:w=720:h=8:color=0xFFE14A@1.0:t=fill,"
        "drawtext=fontfile=arialbd.ttf:text='ON THE ROSTER'"
        ":fontsize=36:fontcolor=0xFFE14A:x=48:y=1000,"
        "drawtext=fontfile=arialbd.ttf:text='2KCOMPLEAGUE.COM'"
        ":fontsize=28:fontcolor=0x4DA3FF:x=48:y=1070[v];"
        f"[1:a]{GAME_BED},atrim=0:6,asetpts=PTS-STARTPTS[a]"
    )
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-f",
            "lavfi",
            "-i",
            "color=c=black:s=1080x1920:r=30:d=6",
            "-ss",
            "30",
            "-t",
            "6",
            "-i",
            str(source),
            "-i",
            str(mark),
            "-filter_complex",
            plate,
            "-map",
            "[v]",
            "-map",
            "[a]",
            "-c:v",
            "libx264",
            "-preset",
            "veryfast",
            "-crf",
            "18",
            "-c:a",
            "aac",
            "-b:a",
            "160k",
            "-shortest",
            "-movflags",
            "+faststart",
            str(dest),
        ],
        cwd=Path(FONTS),
    )


def brand_league_short(source: Path, dest: Path, logo: Path | None = None) -> None:
    brand_short(
        source,
        dest,
        logo=logo or LEAGUE_LOGO,
        name=LEAGUE_NAME,
        url=LEAGUE_URL,
        url_color="0x4DA3FF",
    )


def brand_boxing_short(source: Path, dest: Path, logo: Path | None = None) -> None:
    brand_short(
        source,
        dest,
        logo=logo or BOXING_LOGO,
        name=BOXING_NAME,
        url=BOXING_URL,
        url_color="0xE0C36A",
    )


def stamp_league_logo(image: Path, logo: Path | None = None) -> None:
    mark = logo or LEAGUE_LOGO
    if not mark.exists():
        return
    temporary = image.with_suffix(".logo.jpg")
    _run(
        [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(image),
            "-i",
            str(mark),
            "-filter_complex",
            "[1:v]colorkey=0x000000:0.16:0.08,scale=280:-1[lg];[0:v][lg]overlay=W-w-48:48",
            "-q:v",
            "2",
            "-update",
            "1",
            str(temporary),
        ]
    )
    temporary.replace(image)


def spoken_lines(spoken: str) -> list[str]:
    chunks = [chunk.strip() for chunk in spoken.replace("!", ".").split(".")]
    return [chunk for chunk in chunks if chunk]
