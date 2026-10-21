from dataclasses import replace

from channel_auto.models import ClipPlan, Moment, VideoMeta
from channel_auto.parse import clamp_moment, parse_title
from channel_auto.players import PLAYER_REEL_KINDS, STAT_KINDS
from channel_auto.scorebug import select_coach_film
from channel_auto.script import (
    BOXING_TAGS,
    LEAGUE_TAGS,
    coach_lines,
    coach_names,
    knockout_lines,
    league_lines,
    make_plan,
    player_lines,
    promo_lines,
)


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
    roster: list[str] | None = None,
) -> list[ClipPlan]:
    usable = _usable(meta, moments)
    allowed = [moment for moment in usable if moment.kind == "allowed"]
    highlights = [
        moment
        for moment in usable
        if moment.kind not in {"allowed", "featured"} and moment.kind not in STAT_KINDS
    ]
    stat_plays = [moment for moment in usable if moment.player and moment.kind in PLAYER_REEL_KINDS]
    roster_names = [name for name in (roster or []) if name]
    if not highlights and not allowed and not stat_plays and not roster_names:
        return []
    parsed = parse_title(meta.title)
    source = _source(meta)
    teams = parsed["teams"]
    plans = []
    if highlights:
        plans.append(
            make_plan(
                slug="full",
                kind="full",
                lines=league_lines(teams, parsed["game_number"], None, "play"),
                hashtags=LEAGUE_TAGS,
                moments=pack_moments(highlights, 55),
                source_url=source,
                confident=confident,
            )
        )
    film = select_coach_film(allowed)
    if film:
        team, opponent = coach_names(meta.title, teams)
        points = int(sum(moment.score for moment in film))
        plans.append(
            make_plan(
                slug="coach",
                kind="coach",
                lines=coach_lines(team, opponent, points),
                hashtags=LEAGUE_TAGS + ["#uncs", "#filmstudy"],
                moments=film,
                source_url=source,
                confident=confident,
                hook=f"{team} film vs {opponent}",
            )
        )
    plans.extend(marketing_plans(meta, allowed, parsed, source, confident=confident))
    grouped: dict[str, list[Moment]] = {}
    for moment in stat_plays:
        grouped.setdefault(moment.player, []).append(moment)
    ordered: list[str] = []
    seen: set[str] = set()
    for name in roster_names + list(grouped):
        if name and name not in seen:
            seen.add(name)
            ordered.append(name)
    for player in ordered:
        owned = grouped.get(player, [])
        packed = pack_moments(owned, 240) if owned else []
        if packed:
            lines = player_lines(player, packed)
        else:
            lines = [player, "ON THE ROSTER.", "2K Comp League."]
        plans.append(
            make_plan(
                slug="player-" + _slug(player),
                kind="player",
                lines=lines,
                hashtags=LEAGUE_TAGS + ["#2kcomp", "#playerhighlights"],
                moments=packed,
                source_url=source,
                player=player,
                confident=confident and names_trusted,
                hook=player,
                voice_notice=False,
                kicker=lines[1],
            )
        )
    return plans


def _late_window(moment: Moment, length: float) -> Moment:
    start = max(0.0, moment.end - length)
    if moment.end - start < 4:
        return moment
    return replace(moment, start=start)


def marketing_plans(
    meta: VideoMeta,
    allowed: list[Moment],
    parsed: dict,
    source: str,
    *,
    confident: bool,
) -> list[ClipPlan]:
    if not allowed:
        return []
    teams = parsed["teams"]
    winner = teams[0].upper() if teams else "THE LEAGUE"
    if winner == "W2E":
        winner = "W2E"
    underdog = teams[1].upper() if len(teams) > 1 else "THE FIELD"
    matchup = f"{winner} OVER {underdog}."
    game_number = parsed["game_number"]
    game_line = f"GAME {game_number}." if game_number else "LEAGUE NIGHT."
    threes = [moment for moment in allowed if moment.score >= 3]
    featured = _late_window((threes or allowed)[-1], 8)
    brand_moment = _late_window(threes[-2] if len(threes) > 1 else featured, 9)
    run = [_late_window(moment, 6) for moment in allowed[-4:]]
    tags = LEAGUE_TAGS + ["#2kcomp", "#leaguepass"]
    specs = [
        ("promo-hook", "WHO GOT HIM?" if threes else "AT THE RIM.", [featured], game_line),
        ("promo-run", "THEY KEPT COMING.", run, matchup),
        ("promo-bumper", "THIS IS THE LEAGUE.", [brand_moment], "2KCOMPLEAGUE.COM"),
    ]
    plans = []
    for slug, headline, moments, kicker in specs:
        plans.append(
            make_plan(
                slug=slug,
                kind="promo",
                lines=promo_lines(headline, matchup),
                hashtags=tags,
                moments=moments,
                source_url=source,
                confident=confident,
                hook=headline,
                voice_notice=False,
                kicker=kicker,
            )
        )
    del meta
    return plans


def _slug(value: str) -> str:
    cleaned = "".join(char.lower() if char.isalnum() else "-" for char in value)
    while "--" in cleaned:
        cleaned = cleaned.replace("--", "-")
    return cleaned.strip("-")[:32] or "player"
