import tempfile
import unittest
from pathlib import Path

from channel_auto.config import load_settings, require_channel
from channel_auto.detect import even_windows, knockout_windows, select_peaks
from channel_auto.hud import clean_gamertag, resolve_name
from channel_auto.models import Moment, VideoMeta
from channel_auto.parse import parse_moment_block, parse_roster, parse_title
from channel_auto.pipeline import build_plans
from channel_auto.publish_youtube import privacy_for
from channel_auto.render import thumbnail_lines
from channel_auto.script import AI_LINE, CREDIT
from channel_auto.state import knockout_render_current, should_process


class PlanTests(unittest.TestCase):
    def test_channels_and_playlists(self):
        settings = load_settings()
        self.assertEqual(settings.source_channel_id, "UCFOnI2zXhMeqMDlJWJTmhXw")
        self.assertEqual(settings.destination_channel_id, "UCxR_9dxq1OeOCJqW2-q-78A")
        self.assertEqual(
            {item.id for item in settings.playlists},
            {"PLLhVwQz29NhE", "PLUgImi6WUKkE"},
        )

    def test_source_guard(self):
        require_channel("UCFOnI2zXhMeqMDlJWJTmhXw", "UCFOnI2zXhMeqMDlJWJTmhXw", "Source")
        with self.assertRaises(ValueError):
            require_channel("UCother", "UCFOnI2zXhMeqMDlJWJTmhXw", "Source")

    def test_title_parse(self):
        league = parse_title("w2e over underdogs gm 2")
        self.assertEqual(league["teams"], ["w2e", "underdogs"])
        self.assertEqual(league["game_number"], 2)
        bout = parse_title("Usyk vs Fury")
        self.assertEqual(bout["fighters"], ["Usyk", "Fury"])
        theme = parse_title("They Always Pick High Rated Boxers")
        self.assertEqual(theme["fighters"], [])
        self.assertEqual(theme["teams"], [])

    def test_moment_file(self):
        text = Path(__file__).resolve().parents[1].joinpath("examples", "moments.example.txt").read_text(
            encoding="utf-8"
        )
        roster = parse_roster(text)
        moments = parse_moment_block(text)
        self.assertEqual(roster[0], "OGSportsGamer")
        self.assertEqual(moments[0].player, "OGSportsGamer")
        self.assertEqual(moments[0].kind, "three")
        self.assertAlmostEqual(moments[0].start, 38.0)
        self.assertEqual(moments[1].kind, "dunk")

    def test_gamertag_resolution(self):
        self.assertEqual(clean_gamertag("GAME STATS SmokinSpliffss 0.0 PPG"), "SmokinSpliffss")
        self.assertEqual(resolve_name("OGSportsGamer 14", ["OGSportsGamer"]), "OGSportsGamer")
        self.assertIsNone(resolve_name("random tag here", ["OGSportsGamer"]))
        self.assertEqual(resolve_name("SmokinSpliffss", []), "SmokinSpliffss")

    def test_peaks_keep_their_distance(self):
        levels = [1.0] * 80
        levels[10] = 20
        levels[12] = 18
        levels[50] = 22
        moments = select_peaks(
            levels,
            hop_sec=0.5,
            duration=40,
            min_gap=8,
            pad_before=2,
            pad_after=2,
            top_n=4,
            kind="exchange",
        )
        self.assertGreaterEqual(len(moments), 2)
        self.assertGreaterEqual(moments[1].start - moments[0].start, 8)
        self.assertTrue(all(moment.kind == "exchange" for moment in moments))

    def test_boxing_plan_is_knockouts_only(self):
        meta = VideoMeta("abc", "They Always Pick High Rated Boxers", "UCsrc", 300)
        exchanges = [
            Moment(10, 22, score=3, kind="exchange"),
            Moment(40, 52, score=2, kind="exchange"),
        ]
        self.assertEqual(
            build_plans(meta, exchanges, "knockout_highlight", confident=True, names_trusted=True),
            [],
        )
        plans = build_plans(
            meta,
            [Moment(230, 252, score=5, kind="ko"), Moment(40, 52, score=2, kind="exchange")],
            "knockout_highlight",
            confident=True,
            names_trusted=True,
        )
        self.assertEqual(len(plans), 1)
        self.assertEqual(plans[0].slug, "knockouts")
        self.assertTrue(plans[0].spoken.startswith("KNOCKOUT."))
        self.assertEqual(plans[0].hook, meta.title)
        self.assertNotIn("Voice and commentary", plans[0].tiktok_caption)
        self.assertNotIn("high-rated", plans[0].spoken.lower())
        self.assertIn("#esportsboxingleague", plans[0].tiktok_caption)
        windows = knockout_windows([244, 245, 246, 10, 11], 300)
        self.assertEqual(len(windows), 2)
        self.assertEqual(windows[0].kind, "ko")

    def test_league_player_reels(self):
        meta = VideoMeta("game", "w2e over underdogs gm 2", "UCsrc", 1800)
        moments = [
            Moment(40, 52, score=3, kind="three", player="OGSportsGamer"),
            Moment(140, 152, score=4, kind="dunk", player="SmokinSpliffss"),
            Moment(400, 412, score=2, kind="play", player="OGSportsGamer"),
        ]
        plans = build_plans(meta, moments, "league_highlights", confident=True, names_trusted=True)
        kinds = [plan.kind for plan in plans]
        self.assertIn("full", kinds)
        players = {plan.player for plan in plans if plan.player}
        self.assertEqual(players, {"OGSportsGamer", "SmokinSpliffss"})
        unnamed = build_plans(
            meta,
            [Moment(10, 22, score=2, kind="play")],
            "league_highlights",
            confident=True,
            names_trusted=False,
        )
        self.assertEqual([plan.kind for plan in unnamed], ["full"])

    def test_quiet_video_fallback_is_unlisted(self):
        windows = even_windows(90, 3, 12)
        self.assertEqual(len(windows), 3)
        self.assertEqual(privacy_for(False, "public"), "unlisted")
        self.assertEqual(privacy_for(True, "public"), "public")

    def test_thumbnail_copy_is_short(self):
        first, second = thumbnail_lines("They always pick the high-rated boxer.")
        self.assertLessEqual(len(first), 22)
        self.assertTrue(first.isupper())
        self.assertTrue(second)

    def test_videos_folder(self):
        settings = load_settings()
        self.assertEqual(settings.videos_dir, Path(r"C:\Users\Shadow\Videos\ready for upload"))
        self.assertFalse(settings.upload_youtube)
        self.assertEqual(settings.max_height, 1080)

    def test_skip_rules(self):
        self.assertFalse(should_process({"status": "baseline"}, backfill=False, force=False))
        self.assertTrue(should_process({"status": "baseline"}, backfill=True, force=False))
        self.assertFalse(should_process({"status": "rendered"}, backfill=True, force=False))
        self.assertTrue(should_process({"status": "error", "attempts": 1}, backfill=False, force=False))
        self.assertFalse(should_process({"status": "error", "attempts": 3}, backfill=False, force=False))
        self.assertFalse(
            knockout_render_current(
                {"status": "rendered", "outputs": ["C:/clips/2026-10-18_abc_recap.mp4"]}
            )
        )
        self.assertTrue(
            knockout_render_current(
                {"status": "rendered", "outputs": ["C:/clips/2026-10-19_abc_knockouts.mp4"]}
            )
        )


class RenderTests(unittest.TestCase):
    def test_vertical_export(self):
        import subprocess

        from channel_auto.render import (
    LEAGUE_LOGO,
    brand_league_short,
    probe_duration,
    probe_size,
    render_clip,
    thumbnail_lines,
    write_thumbnail,
)

        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / "source.mp4"
            subprocess.run(
                [
                    "ffmpeg",
                    "-y",
                    "-hide_banner",
                    "-loglevel",
                    "error",
                    "-f",
                    "lavfi",
                    "-i",
                    "testsrc=size=1280x720:rate=30:duration=4",
                    "-f",
                    "lavfi",
                    "-i",
                    "sine=frequency=440:duration=4",
                    "-shortest",
                    "-c:v",
                    "libx264",
                    "-pix_fmt",
                    "yuv420p",
                    "-c:a",
                    "aac",
                    str(source),
                ],
                check=True,
                capture_output=True,
            )
            dest = root / "out" / "clip.mp4"
            render_clip(
                source,
                [Moment(0.2, 2.0, score=1), Moment(2.0, 3.8, score=1)],
                "W2E and underdogs.",
                ["Full highlight.", "Watch the burst."],
                dest,
            )
            self.assertTrue(dest.exists())
            self.assertEqual(probe_size(dest), (1080, 1920))
            self.assertGreater(probe_duration(dest), 2.5)
            self.assertLess(probe_duration(dest), 5)
            thumb = root / "out" / "thumb.jpg"
            write_thumbnail(source, 1.0, "W2E and underdogs.", "2KCL", thumb)
            self.assertEqual(probe_size(thumb), (1920, 1080))
            self.assertLess(thumb.stat().st_size, 2_000_000)
            branded = root / "out" / "branded.mp4"
            self.assertTrue(LEAGUE_LOGO.exists())
            brand_league_short(dest, branded)
            self.assertEqual(probe_size(branded), (1080, 1920))


if __name__ == "__main__":
    unittest.main()
