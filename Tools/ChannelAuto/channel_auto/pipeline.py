import json
import subprocess
import traceback
from dataclasses import replace
from datetime import date
from pathlib import Path

from channel_auto.config import Settings, require_channel
from channel_auto.detect import even_windows, extract_wav, find_knockouts, rms_windows, select_peaks
from channel_auto.hud import extract_frame, ocr_available, read_player_name
from channel_auto.models import Moment, VideoMeta
from channel_auto.parse import moments_from_chapters, parse_moment_block, parse_roster
from channel_auto.plan import build_league, build_undisputed
from channel_auto.players import scan_player_events
from channel_auto.scorebug import scan_opponent_scores, scoring_events
from channel_auto.publish_youtube import credentials_ready, upload_short
from channel_auto.render import (
    BOXING_LOGO,
    BOXING_URL,
    LEAGUE_URL,
    brand_boxing_short,
    brand_league_short,
    brand_player,
    brand_promo,
    render_clip,
    spoken_lines,
    stamp_league_logo,
    write_coach_still,
    write_thumbnail,
)
from channel_auto.state import knockout_render_current, load_state, save_state, should_process, touch_lock
from channel_auto.voice import synthesize
from channel_auto.youtube_io import download_video, fetch_meta, list_playlist, media_path


def cue_path(settings: Settings, video_id: str) -> Path:
    safe = video_id.replace("/", "_")
    return settings.inbox / "moments" / f"{safe}.txt"


def load_cues(meta: VideoMeta, settings: Settings) -> tuple[list[Moment], list[str], bool]:
    sidecar = cue_path(settings, meta.video_id)
    text = sidecar.read_text(encoding="utf-8") if sidecar.exists() else meta.description
    roster = parse_roster(text)
    moments = parse_moment_block(text)
    trusted = bool(moments)
    if not moments:
        moments = moments_from_chapters(meta.chapters)
        trusted = any(moment.player for moment in moments)
    return moments, roster, trusted


def detect_moments(video: Path, meta: VideoMeta, profile: str, work: Path) -> tuple[list[Moment], bool]:
    wav = work / "audio.wav"
    extract_wav(video, wav)
    levels = rms_windows(wav)
    kind = "play"
    moments = select_peaks(
        levels,
        hop_sec=0.5,
        duration=meta.duration,
        min_gap=18,
        pad_before=6,
        pad_after=6,
        top_n=8,
        kind=kind,
    )
    if moments:
        return moments, True
    return even_windows(meta.duration, 3, 12), False


def assign_players(
    video: Path,
    moments: list[Moment],
    roster: list[str],
    work: Path,
) -> list[Moment]:
    if not ocr_available():
        return moments
    named: list[Moment] = []
    for index, moment in enumerate(moments):
        if moment.player:
            named.append(moment)
            continue
        frame = work / f"frame_{index:02d}.jpg"
        stamp = min(moment.end - 0.2, moment.start + moment.duration * 0.6)
        try:
            extract_frame(video, stamp, frame)
            player = read_player_name(frame, roster, work)
        except Exception as exc:
            print(f"Gamertag read failed at {stamp:.1f}s: {exc}")
            player = None
        named.append(replace(moment, player=player or moment.player))
    return named


def build_plans(meta: VideoMeta, moments: list[Moment], profile: str, *, confident: bool, names_trusted: bool):
    if profile == "knockout_highlight":
        return build_undisputed(meta, moments, confident=confident)
    if profile == "league_highlights":
        return build_league(meta, moments, confident=confident, names_trusted=names_trusted)
    raise ValueError(f"Unknown profile {profile}")


def _file_slug(video_id: str, slug: str) -> str:
    safe = video_id.replace("-", "")
    return f"{date.today().isoformat()}_{safe}_{slug}"


def _folder_name(title: str, video_id: str) -> str:
    cleaned = "".join(char if char.isalnum() or char in " -_" else " " for char in title)
    cleaned = " ".join(cleaned.split()).strip()[:60]
    return cleaned or video_id.replace("-", "")


def _badge(profile: str, player: str | None, kind: str) -> str:
    if kind == "coach":
        return "UNCS FILM"
    if kind == "player":
        return "2K COMP LEAGUE"
    if player:
        return "PLAYER HIGHLIGHT"
    if profile == "knockout_highlight":
        return "KNOCKOUT"
    if kind == "full":
        return "2K COMP LEAGUE"
    return "2K COMP LEAGUE"


def render_plans(settings: Settings, video: Path, meta: VideoMeta, profile: str, plans) -> list[dict]:
    out_dir = settings.videos_dir / _folder_name(meta.title, meta.video_id)
    out_dir.mkdir(parents=True, exist_ok=True)
    voice_name = settings.voice_for(profile)
    rendered = []
    for plan in plans:
        stem = _file_slug(meta.video_id, plan.slug)
        clip_dir = out_dir
        if plan.kind == "player" and plan.player:
            clip_dir = out_dir / "players" / _folder_name(plan.player, plan.player)
            dest = clip_dir / "highlight.mp4"
        else:
            dest = clip_dir / f"{stem}.mp4"
        thumb = dest.with_suffix(".jpg")
        scratch = settings.inbox / "work" / meta.video_id.replace("-", "") / stem
        voice_path = scratch / "voice.mp3"
        has_voice = False
        if profile != "knockout_highlight" and plan.kind not in {"promo", "player"}:
            has_voice = synthesize(plan.spoken, voice_name, voice_path)
        print(f"Cutting {plan.player or plan.slug}")
        render_clip(
            video,
            plan.moments,
            plan.hook,
            spoken_lines(plan.spoken),
            dest,
            voice_path if has_voice else None,
            work_dir=scratch,
            duck_voice=plan.kind == "player",
        )
        best = max(plan.moments, key=lambda moment: moment.score)
        if profile == "knockout_highlight":
            stamp = min(best.end - 1.0, best.start + 14.0)
        elif plan.kind == "promo":
            stamp = max(0.0, best.end - 3.0)
        else:
            stamp = best.start + min(1.0, best.duration * 0.4)
        write_thumbnail(video, stamp, plan.hook, _badge(profile, plan.player, plan.kind), thumb)
        caption = plan.tiktok_caption
        description = plan.youtube_description
        if plan.kind == "promo":
            brand_promo(dest, dest, plan.hook, plan.kicker or "2KCOMPLEAGUE.COM")
            stamp_league_logo(thumb)
            brand_line = f"2K Comp League\n{LEAGUE_URL.lower()}"
            caption = f"{caption}\n\n{brand_line}"
            description = f"{description}\n\n{brand_line}"
        elif plan.kind == "player":
            brand_player(dest, dest, plan.player or plan.hook, plan.kicker or "2K COMP LEAGUE")
            stamp_league_logo(thumb)
            brand_line = f"2K Comp League\n{LEAGUE_URL.lower()}"
            caption = f"{caption}\n\n{brand_line}"
            description = f"{description}\n\n{brand_line}"
        elif profile == "league_highlights":
            brand_league_short(dest, dest)
            stamp_league_logo(thumb)
            brand_line = f"2K Comp League\n{LEAGUE_URL.lower()}"
            caption = f"{caption}\n\n{brand_line}"
            description = f"{description}\n\n{brand_line}"
        elif profile == "knockout_highlight":
            brand_boxing_short(dest, dest)
            stamp_league_logo(thumb, logo=BOXING_LOGO)
            brand_line = f"Esports Boxing League\n{BOXING_URL.lower()}"
            caption = f"{caption}\n\n{brand_line}"
            description = f"{description}\n\n{brand_line}"
        (dest.with_suffix(".tiktok.txt")).write_text(caption, encoding="utf-8")
        (dest.with_suffix(".youtube.txt")).write_text(plan.title + "\n\n" + description, encoding="utf-8")
        if plan.kind == "coach":
            notes = [plan.spoken, ""]
            for index, moment in enumerate(plan.moments, start=1):
                still = out_dir / f"{stem}-still-{index:02d}.jpg"
                write_coach_still(video, max(0.0, moment.end - 4.0), moment.note.upper(), still)
                notes.append(f"{index}. {moment.note}  (source {moment.start:.0f}s)")
            (out_dir / f"{stem}-notes.txt").write_text("\n".join(notes) + "\n", encoding="utf-8")
        youtube_url = ""
        if settings.upload_youtube and credentials_ready(settings):
            youtube_url = upload_short(
                settings,
                dest,
                plan.title,
                description,
                confident=plan.confident,
                thumbnail=thumb,
            )
            print(f"Uploaded to {settings.destination_handle}: {youtube_url}")
        rendered.append(
            {
                "file": str(dest),
                "thumbnail": str(thumb),
                "kind": plan.kind,
                "player": plan.player,
                "title": plan.title,
                "confident": plan.confident,
                "youtube_url": youtube_url,
            }
        )
    if profile == "knockout_highlight":
        _retire_commentary(out_dir, meta.video_id)
    return rendered


def _retire_commentary(out_dir: Path, video_id: str) -> None:
    slug = video_id.replace("-", "")
    for path in out_dir.glob(f"*{slug}*"):
        if "_knockouts" in path.name:
            continue
        if path.suffix.lower() in {".mp4", ".jpg", ".txt"}:
            path.unlink(missing_ok=True)


def process_video(settings: Settings, video_id: str, playlist_id: str, profile: str) -> dict:
    meta = fetch_meta(video_id)
    require_channel(meta.channel_id, settings.source_channel_id, "Source")
    if meta.duration <= 0 or meta.duration > settings.max_duration_sec:
        raise RuntimeError(f"{video_id} is {meta.duration:.0f}s, outside the clip window.")
    settings.inbox.mkdir(parents=True, exist_ok=True)
    video = media_path(settings.inbox, meta.video_id)
    if not video.exists():
        download_video(meta.video_id, video, settings.max_height)
    work = settings.inbox / "work" / meta.video_id.replace("-", "")
    work.mkdir(parents=True, exist_ok=True)
    moments, roster, names_trusted = load_cues(meta, settings)
    confident = True
    if profile == "knockout_highlight":
        labeled = [moment for moment in moments if moment.kind in {"ko", "knockdown", "tko"}]
        moments = labeled or find_knockouts(video, meta.duration)
        confident = bool(moments)
    elif not moments:
        moments, confident = detect_moments(video, meta, profile, work)
        if profile == "league_highlights":
            moments = assign_players(video, moments, roster, work)
            names_trusted = bool(roster) and any(moment.player for moment in moments)
    if profile == "league_highlights":
        try:
            moments = list(moments) + scoring_events(scan_opponent_scores(video), meta.duration)
        except (OSError, subprocess.CalledProcessError, ValueError) as exc:
            print(f"Scoreboard read failed: {exc}")
        try:
            card_events = scan_player_events(video, meta.duration)
            if card_events:
                moments = list(moments) + card_events
                names_trusted = True
        except (OSError, subprocess.CalledProcessError, ValueError, ImportError) as exc:
            print(f"Player card read failed: {exc}")
    plans = build_plans(meta, moments, profile, confident=confident, names_trusted=names_trusted)
    if not plans:
        if profile == "knockout_highlight":
            raise RuntimeError(f"No knockout graphic found in {meta.title}.")
        raise RuntimeError(f"No clips could be planned for {video_id}.")
    clips = render_plans(settings, video, meta, profile, plans)
    player_clips = [clip for clip in clips if clip["kind"] == "player"]
    note = ""
    if profile == "league_highlights" and not player_clips:
        note = (
            "No per-player cuts. The player card did not show a stat change, "
            "or the gamertag could not be read."
        )
    manifest = {
        "video_id": meta.video_id,
        "title": meta.title,
        "source_channel": settings.source_name,
        "destination_channel": settings.destination_handle,
        "playlist_id": playlist_id,
        "profile": profile,
        "clips": clips,
        "player_note": note,
    }
    out_dir = settings.videos_dir / _folder_name(meta.title, meta.video_id)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    if note:
        print(note)
    return manifest


def poll(settings: Settings, *, backfill: bool, force: bool = False) -> int:
    state = load_state(settings.state_path)
    found: list[tuple[str, dict, str]] = []
    for playlist in settings.playlists:
        try:
            entries = list_playlist(playlist.id)
        except Exception as exc:
            print(f"Could not read {playlist.name}: {exc}")
            continue
        print(f"{playlist.name}: {len(entries)} video(s)")
        for entry in entries:
            found.append((playlist.id, entry, playlist.profile))
    if not state.get("baselined") and not backfill and not force:
        for playlist_id, entry, _profile in found:
            state["videos"].setdefault(
                entry["id"],
                {
                    "status": "baseline",
                    "playlist_id": playlist_id,
                    "title": entry.get("title") or "",
                    "attempts": 0,
                },
            )
        state["baselined"] = True
        save_state(settings.state_path, state)
        print(
            "Marked the videos already in the playlists as seen. "
            "New uploads will be clipped. Run backfill to clip the videos that are already there."
        )
        return 0
    state["baselined"] = True
    done = 0
    for playlist_id, entry, profile in found:
        video_id = entry["id"]
        record = state["videos"].get(video_id)
        stale_boxing = (
            profile == "knockout_highlight"
            and record is not None
            and record.get("status") in {"rendered", "uploaded"}
            and not knockout_render_current(record)
        )
        stale_coach = (
            profile == "league_highlights"
            and record is not None
            and record.get("status") in {"rendered", "uploaded"}
            and record.get("coach") not in {"ready", "skipped"}
        )
        if not should_process(record, backfill=backfill, force=force) and not stale_boxing and not stale_coach:
            continue
        touch_lock(settings.lock_path)
        print(f"Clipping {entry.get('title') or video_id} from {settings.source_name}")
        try:
            manifest = process_video(settings, video_id, playlist_id, profile)
            uploaded = any(clip.get("youtube_url") for clip in manifest["clips"])
            state["videos"][video_id] = {
                "status": "uploaded" if uploaded else "rendered",
                "playlist_id": playlist_id,
                "title": manifest["title"],
                "outputs": [clip["file"] for clip in manifest["clips"]],
                "attempts": 0,
                "coach": (
                    "ready"
                    if any(clip.get("kind") == "coach" for clip in manifest["clips"])
                    else "skipped"
                    if profile == "league_highlights"
                    else ""
                ),
                "marketing": (
                    "ready"
                    if any(clip.get("kind") == "promo" for clip in manifest["clips"])
                    else "skipped"
                    if profile == "league_highlights"
                    else ""
                ),
                "players": (
                    "ready"
                    if any(clip.get("kind") == "player" for clip in manifest["clips"])
                    else "skipped"
                    if profile == "league_highlights"
                    else ""
                ),
            }
            done += 1
        except Exception as exc:
            attempts = int((record or {}).get("attempts", 0)) + 1
            status = "failed" if attempts >= 3 else "error"
            state["videos"][video_id] = {
                "status": status,
                "playlist_id": playlist_id,
                "title": entry.get("title") or "",
                "error": str(exc),
                "attempts": attempts,
            }
            print(f"Failed {video_id}: {exc}")
            traceback.print_exc()
        save_state(settings.state_path, state)
    return done
