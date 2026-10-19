import re
from pathlib import Path

from channel_auto.models import VideoMeta

_SAFE = re.compile(r"[^A-Za-z0-9_-]")


def watch_url(video_id: str) -> str:
    return f"https://www.youtube.com/watch?v={video_id}"


def media_path(inbox: Path, video_id: str) -> Path:
    safe = _SAFE.sub("_", video_id)
    return inbox / f"v{safe}.mp4"


def list_playlist(playlist_id: str) -> list[dict]:
    import yt_dlp

    url = f"https://www.youtube.com/playlist?list={playlist_id}"
    options = {"extract_flat": "in_playlist", "quiet": True, "skip_download": True}
    with yt_dlp.YoutubeDL(options) as downloader:
        info = downloader.extract_info(url, download=False)
    entries = []
    for entry in info.get("entries") or []:
        if not entry or not entry.get("id"):
            continue
        entries.append(
            {
                "id": entry["id"],
                "title": entry.get("title") or "",
                "duration": entry.get("duration"),
            }
        )
    return entries


def fetch_meta(video_id: str) -> VideoMeta:
    import yt_dlp

    options = {"quiet": True, "skip_download": True}
    with yt_dlp.YoutubeDL(options) as downloader:
        info = downloader.extract_info(watch_url(video_id), download=False)
    return VideoMeta(
        video_id=info["id"],
        title=info.get("title") or info["id"],
        channel_id=info.get("channel_id") or "",
        duration=float(info.get("duration") or 0),
        description=info.get("description") or "",
        chapters=list(info.get("chapters") or []),
        webpage_url=info.get("webpage_url") or watch_url(info["id"]),
    )


def download_video(video_id: str, dest: Path, max_height: int) -> Path:
    import yt_dlp

    dest.parent.mkdir(parents=True, exist_ok=True)
    options = {
        "format": (
            f"bv*[height<={max_height}][ext=mp4][vcodec^=avc1]+ba[ext=m4a]/"
            f"bv*[height<={max_height}][ext=mp4]+ba[ext=m4a]/"
            f"b[height<={max_height}]/b"
        ),
        "merge_output_format": "mp4",
        "outtmpl": str(dest.with_suffix("")) + ".%(ext)s",
        "quiet": False,
        "noplaylist": True,
        "retries": 3,
    }
    with yt_dlp.YoutubeDL(options) as downloader:
        downloader.download([watch_url(video_id)])
    if dest.exists():
        return dest
    matches = list(dest.parent.glob(dest.stem + ".*"))
    videos = [path for path in matches if path.suffix.lower() in {".mp4", ".mkv", ".webm"}]
    if not videos:
        raise FileNotFoundError(f"Download finished without a video file for {video_id}")
    winner = videos[0]
    if winner != dest:
        winner.replace(dest)
    return dest
