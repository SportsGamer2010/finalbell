import argparse
import time

from channel_auto.config import load_settings
from channel_auto.models import Moment, VideoMeta
from channel_auto.parse import parse_title
from channel_auto.pipeline import build_plans, poll, process_video
from channel_auto.publish_youtube import build_service, credentials_ready
from channel_auto.state import acquire_lock, load_state, release_lock
from channel_auto.youtube_io import list_playlist


def _preview() -> None:
    samples = [
        (
            "knockout_highlight",
            VideoMeta(
                video_id="-sry2QcWz68",
                title="They Always Pick High Rated Boxers",
                channel_id="UCFOnI2zXhMeqMDlJWJTmhXw",
                duration=300,
                webpage_url="https://www.youtube.com/watch?v=-sry2QcWz68",
            ),
            [
                Moment(230, 252, score=5, kind="ko"),
            ],
        ),
        (
            "league_highlights",
            VideoMeta(
                video_id="wlRB7gRcNeE",
                title="w2e over underdogs gm 2",
                channel_id="UCFOnI2zXhMeqMDlJWJTmhXw",
                duration=1800,
                webpage_url="https://www.youtube.com/watch?v=wlRB7gRcNeE",
            ),
            [
                Moment(40, 52, score=3, kind="three", player="OGSportsGamer"),
                Moment(140, 152, score=4, kind="dunk", player="SmokinSpliffss"),
                Moment(400, 412, score=2, kind="play", player="OGSportsGamer"),
            ],
        ),
    ]
    for profile, meta, moments in samples:
        print(f"\n{meta.title}")
        print(parse_title(meta.title))
        plans = build_plans(meta, moments, profile, confident=True, names_trusted=True)
        for plan in plans:
            print(f"  {plan.kind:8} {plan.title}")
            print(f"           {plan.spoken}")


def _status() -> None:
    settings = load_settings()
    state = load_state(settings.state_path)
    counts: dict[str, int] = {}
    for record in state["videos"].values():
        status = record.get("status", "unknown")
        counts[status] = counts.get(status, 0) + 1
    print(f"Source: {settings.source_name} {settings.source_handle}")
    print(f"Destination: {settings.destination_handle}")
    print(f"Baselined: {state.get('baselined')}")
    if not counts:
        print("No videos tracked yet.")
        return
    for status, count in sorted(counts.items()):
        print(f"  {status}: {count}")


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(prog="channel_auto")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("watch", help="Keep watching the GameAthlete playlists")
    sub.add_parser("once", help="Check the playlists one time")
    sub.add_parser("backfill", help="Clip the videos already in the playlists")
    sub.add_parser("preview", help="Print sample clip plans without downloading")
    sub.add_parser("status", help="Show which videos have been clipped")
    sub.add_parser("playlists", help="Print the videos currently in the source playlists")
    sub.add_parser("auth", help="Sign in the TechBeatsxGames YouTube channel")
    process = sub.add_parser("process", help="Clip one GameAthlete video")
    process.add_argument("--video", required=True)
    process.add_argument("--playlist", default="")
    args = parser.parse_args(argv)
    settings = load_settings()

    if args.command == "preview":
        _preview()
        return
    if args.command == "status":
        _status()
        return
    if args.command == "playlists":
        for playlist in settings.playlists:
            print(f"\n{playlist.name} ({playlist.profile})")
            for entry in list_playlist(playlist.id):
                print(f"  {entry['id']} | {entry.get('duration')} | {entry['title']}")
        return
    if args.command == "auth":
        if not credentials_ready(settings):
            raise SystemExit(
                "Put the Google OAuth file at Tools/ChannelAuto/secrets/client_secret.json first."
            )
        service = build_service(settings)
        response = service.channels().list(part="snippet", mine=True).execute()
        item = (response.get("items") or [{}])[0]
        snippet = item.get("snippet") or {}
        print(f"Signed in as {snippet.get('title')} ({item.get('id')})")
        print(f"Uploads are allowed only for {settings.destination_handle} ({settings.destination_channel_id}).")
        return
    if args.command == "process":
        playlist_id = args.playlist
        profile = ""
        if playlist_id:
            playlist = settings.playlist_for(playlist_id)
            if playlist is None:
                raise SystemExit(f"Unknown playlist {playlist_id}")
            profile = playlist.profile
        else:
            for item in settings.playlists:
                entries = list_playlist(item.id)
                if any(entry["id"] == args.video for entry in entries):
                    playlist_id = item.id
                    profile = item.profile
                    break
            if not profile:
                raise SystemExit(
                    "That video is not in the configured GameAthlete playlists. Pass --playlist."
                )
        manifest = process_video(settings, args.video, playlist_id, profile)
        print(f"Wrote {len(manifest['clips'])} clip(s) to {settings.videos_dir}.")
        return

    acquire_lock(settings.lock_path)
    try:
        if args.command == "backfill":
            count = poll(settings, backfill=True)
            print(f"Clipped {count} video(s).")
            return
        once = args.command == "once"
        while True:
            count = poll(settings, backfill=False)
            print(f"Clipped {count} new video(s).")
            if once:
                return
            time.sleep(settings.poll_seconds)
    finally:
        release_lock(settings.lock_path)


if __name__ == "__main__":
    main()
