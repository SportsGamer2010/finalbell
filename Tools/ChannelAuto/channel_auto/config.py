import json
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "config" / "channel.json"

PROFILES = {"knockout_highlight", "league_highlights"}


@dataclass(frozen=True)
class PlaylistSpec:
    id: str
    name: str
    profile: str
    url: str


@dataclass(frozen=True)
class Settings:
    source_name: str
    source_handle: str
    source_channel_id: str
    destination_name: str
    destination_handle: str
    destination_channel_id: str
    poll_seconds: int
    max_height: int
    videos_dir: Path
    max_duration_sec: int
    youtube_privacy: str
    upload_youtube: bool
    voices: dict[str, str]
    playlists: list[PlaylistSpec]
    root: Path = ROOT

    @property
    def inbox(self) -> Path:
        return self.root / "inbox"

    @property
    def outbox(self) -> Path:
        return self.root / "outbox"

    @property
    def state_path(self) -> Path:
        return self.root / "state" / "seen.json"

    @property
    def lock_path(self) -> Path:
        return self.root / "state" / "watch.lock"

    @property
    def secrets(self) -> Path:
        return self.root / "secrets"

    def voice_for(self, profile: str) -> str:
        return self.voices.get(profile, "en-US-GuyNeural")

    def playlist_for(self, playlist_id: str) -> PlaylistSpec | None:
        for playlist in self.playlists:
            if playlist.id == playlist_id:
                return playlist
        return None


def load_settings(path: Path | None = None) -> Settings:
    payload = json.loads((path or CONFIG_PATH).read_text(encoding="utf-8"))
    playlists = []
    for item in payload["playlists"]:
        profile = item["profile"]
        if profile not in PROFILES:
            raise ValueError(f"Unknown profile {profile}")
        playlists.append(
            PlaylistSpec(
                id=item["id"],
                name=item["name"],
                profile=profile,
                url=item["url"],
            )
        )
    destination = payload["destination"]
    source = payload["source"]
    videos_dir = Path(payload.get("videos_dir") or (Path.home() / "Videos" / "TechBeats"))
    if not videos_dir.is_absolute():
        videos_dir = ROOT / videos_dir
    return Settings(
        source_name=source["name"],
        source_handle=source["handle"],
        source_channel_id=source["channel_id"],
        destination_name=destination["name"],
        destination_handle=destination["handle"],
        destination_channel_id=destination["channel_id"],
        poll_seconds=int(payload.get("poll_seconds", 600)),
        max_height=int(payload.get("max_height", 1080)),
        videos_dir=videos_dir,
        max_duration_sec=int(payload.get("max_duration_sec", 10800)),
        youtube_privacy=payload.get("youtube_privacy", "unlisted"),
        upload_youtube=bool(payload.get("upload_youtube", True)),
        voices=dict(payload.get("voices", {})),
        playlists=playlists,
    )


def require_channel(actual: str, expected: str, role: str) -> None:
    if actual != expected:
        raise ValueError(f"{role} channel is {actual}. Expected {expected}.")
