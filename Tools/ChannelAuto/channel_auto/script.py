from channel_auto.models import ClipPlan, Moment

AI_LINE = "Voice and commentary are AI-generated."
CREDIT = "Footage: GameAthlete."

BOXING_TAGS = ["#esportsboxingleague", "#esbl", "#knockout", "#undisputed", "#fyp"]
LEAGUE_TAGS = ["#2kcompleague", "#2kcl", "#nba2k", "#2khighlights", "#fyp"]

CALLS = {
    "dunk": "He threw it down.",
    "three": "That's from deep.",
    "block": "He sent it back.",
    "steal": "He took it.",
    "ankle": "He sat him down.",
    "alley": "They threw it up.",
    "knockdown": "He is hurt. The round just turned.",
    "ko": "That is the finish.",
    "exchange": "Big exchange.",
}


def _join(lines: list[str]) -> str:
    spoken = " ".join(line.strip() for line in lines if line.strip())
    return spoken[:500]


def knockout_lines(title: str) -> list[str]:
    del title
    return ["KNOCKOUT."]


def league_lines(
    teams: list[str],
    game_number: int | None,
    player: str | None,
    kind: str,
) -> list[str]:
    if len(teams) >= 2:
        matchup = f"{teams[0]} and {teams[1]}"
    else:
        matchup = "This league game"
    game_bit = f", game {game_number}" if game_number else ""
    if player:
        lines = [
            f"{player}.",
            CALLS.get(kind, "This is the stretch that got him loose."),
            f"His cut from {matchup}{game_bit}.",
        ]
        return lines
    lines = [
        f"{matchup}{game_bit}.",
        "Full highlight. These are the stretches that decided it.",
        CALLS.get(kind, "Watch the burst."),
    ]
    return lines


def youtube_title(hook: str) -> str:
    suffix = " #shorts"
    base = hook.strip().rstrip(".")
    room = 100 - len(suffix)
    if len(base) > room:
        base = base[: room - 1].rstrip() + "…"
    return base + suffix


def youtube_description(spoken: str, source_url: str, *, voice_notice: bool = True) -> str:
    parts = [spoken]
    if voice_notice:
        parts.append(AI_LINE)
    parts.extend([CREDIT, f"Full video: {source_url}"])
    return "\n\n".join(parts)


def tiktok_caption(hook: str, hashtags: list[str], *, voice_notice: bool = True) -> str:
    tags = " ".join(hashtags)
    parts = [hook.strip()]
    if voice_notice:
        parts.append(AI_LINE)
    parts.extend([CREDIT, tags])
    return "\n\n".join(parts)


def make_plan(
    *,
    slug: str,
    kind: str,
    lines: list[str],
    hashtags: list[str],
    moments: list[Moment],
    source_url: str,
    player: str | None = None,
    confident: bool = True,
    hook: str | None = None,
    voice_notice: bool = True,
) -> ClipPlan:
    shown = (hook or lines[0]).strip()
    spoken = _join(lines)
    return ClipPlan(
        slug=slug,
        kind=kind,
        title=youtube_title(shown),
        hook=shown,
        spoken=spoken,
        tiktok_caption=tiktok_caption(shown, hashtags, voice_notice=voice_notice),
        youtube_description=youtube_description(spoken, source_url, voice_notice=voice_notice),
        moments=moments,
        hashtags=hashtags,
        player=player,
        confident=confident,
    )
