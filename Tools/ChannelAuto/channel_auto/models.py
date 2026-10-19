from dataclasses import dataclass, field


@dataclass(frozen=True)
class Moment:
    start: float
    end: float
    score: float = 0.0
    kind: str = "play"
    player: str | None = None
    note: str = ""

    @property
    def duration(self) -> float:
        return max(0.0, self.end - self.start)


@dataclass(frozen=True)
class VideoMeta:
    video_id: str
    title: str
    channel_id: str
    duration: float
    description: str = ""
    chapters: list[dict] = field(default_factory=list)
    webpage_url: str = ""


@dataclass(frozen=True)
class ClipPlan:
    slug: str
    kind: str
    title: str
    hook: str
    spoken: str
    tiktok_caption: str
    youtube_description: str
    moments: list[Moment]
    hashtags: list[str]
    player: str | None = None
    confident: bool = True
