import re

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


def coach_names(title: str, teams: list[str]) -> tuple[str, str]:
    folded = title.lower()
    if "unc" in folded or "underdog" in folded:
        coached = "UNCS"
    elif len(teams) >= 2:
        coached = teams[1]
    else:
        coached = "Underdogs"
    opponent = teams[0] if teams else "the opponent"
    if opponent.lower() == "w2e":
        opponent = "W2E"
    return coached, opponent


def coach_lines(team: str, opponent: str, points: int) -> list[str]:
    return [
        f"{team}, this is the end of the game.",
        f"These are {points} points you gave {opponent}.",
        "Do not watch the make. Watch who was late.",
        "Do not send all five to the ball. Somebody stays home on the shooter.",
        "Close out short and stay down.",
        "Put a body on their big before you look at the ball.",
    ]


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


def player_lines(name: str, moments: list) -> list[str]:
    notes = " ".join(moment.note for moment in moments)
    points = sum(int(value) for value in re.findall(r"\+(\d+) PTS", notes))
    chips = []
    if points:
        chips.append(f"{points} PTS")
    for label in ("REB", "AST", "STL", "BLK"):
        count = notes.count(label)
        if count:
            chips.append(f"{count} {label}")
    dunks = notes.count("DUNK")
    if dunks:
        chips.insert(0, f"{dunks} DUNK")
    if chips:
        line = "  ".join(chips)
    elif any(moment.kind == "featured" for moment in moments):
        line = "ON THE FLOOR."
    else:
        line = "League highlights."
    return [name, line, "2K Comp League."]


def promo_lines(headline: str, matchup: str) -> list[str]:
    return [headline, matchup, "2K Comp League."]


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
    kicker: str = "",
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
        kicker=kicker,
    )
