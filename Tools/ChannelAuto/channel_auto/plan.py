from dataclasses import replace

from channel_auto.models import ClipPlan, Moment, VideoMeta
from channel_auto.parse import clamp_moment, parse_title
from channel_auto.script import BOXING_TAGS, LEAGUE_TAGS, knockout_lines, league_lines, make_plan


def pack_moments(moments: list[Moment], limit: float) -> list[Moment]:
    ranked = sorted(moments, key=lambda moment: moment.score, reverse=True)
    picked: list[Moment] = []
    total = 0.0
    for moment in ranked:
        room = limit - total
        if room < 4:
            break
        if moment.duration > room:
            moment = replace(moment, end=moment.start + room)
        picked.append(moment)
        total += moment.duration
    picked.sort(key=lambda moment: moment.start)
    return picked


def _usable(meta: VideoMeta, moments: list[Moment]) -> list[Moment]:
    clamped = []
    for moment in moments:
        fitted = clamp_moment(moment, meta.duration)
        if fitted is not None:
            clamped.append(fitted)
    return clamped


def _source(meta: VideoMeta) -> str:
    return meta.webpage_url or f"https://www.youtube.com/watch?v={meta.video_id}"


def build_undisputed(meta: VideoMeta, moments: list[Moment], *, confident: bool) -> list[ClipPlan]:
    knockouts = [moment for moment in _usable(meta, moments) if moment.kind in {"ko", "knockdown", "tko"}]
    if not knockouts:
        return []
    return [
        make_plan(
            slug="knockouts",
            kind="full",
            lines=knockout_lines(meta.title),
            hashtags=BOXING_TAGS,
            moments=pack_moments(knockouts, 58),
            source_url=_source(meta),
            confident=confident,
            hook=meta.title,
            voice_notice=False,
        )
    ]


def build_league(
    meta: VideoMeta,
    moments: list[Moment],
    *,
    confident: bool,
    names_trusted: bool,
) -> list[ClipPlan]:
    usable = _usable(meta, moments)
    if not usable:
        return []
    parsed = parse_title(meta.title)
    source = _source(meta)
    teams = parsed["teams"]
    plans = [
        make_plan(
            slug="full",
            kind="full",
            lines=league_lines(teams, parsed["game_number"], None, "play"),
            hashtags=LEAGUE_TAGS,
            moments=pack_moments(usable, 55),
            source_url=source,
            confident=confident,
        )
    ]
    grouped: dict[str, list[Moment]] = {}
    for moment in usable:
        if moment.player:
            grouped.setdefault(moment.player, []).append(moment)
    for player, owned in grouped.items():
        packed = pack_moments(owned, 40)
        if not packed:
            continue
        slug = "player-" + _slug(player)
        plans.append(
            make_plan(
                slug=slug,
                kind="player",
                lines=league_lines(teams, parsed["game_number"], player, packed[0].kind),
                hashtags=LEAGUE_TAGS,
                moments=packed,
                source_url=source,
                player=player,
                confident=confident and names_trusted,
            )
        )
    return plans


def _slug(value: str) -> str:
    cleaned = "".join(char.lower() if char.isalnum() else "-" for char in value)
    while "--" in cleaned:
        cleaned = cleaned.replace("--", "-")
    return cleaned.strip("-")[:32] or "player"
