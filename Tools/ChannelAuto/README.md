# TechBeats clip watcher

This watches two GameAthlete playlists. When a video is added, it builds vertical clips. Nothing is uploaded. Finished videos and 1920×1080 thumbnails are saved in `C:\Users\Shadow\Videos\ready for upload`. Boxing shorts are knockout-only and branded for esportsboxingleague.com. 2K Comp League shorts carry the league logo, an AI voice, and 2kcompleague.com.

`@TechBeatsGames` is not a live handle. The channel that is live is `@TechBeatsxGames`.

| Role | Channel | Id |
| --- | --- | --- |
| Source footage | [@gameathlete](https://www.youtube.com/@gameathlete) | `UCFOnI2zXhMeqMDlJWJTmhXw` |
| Posted clips | [@TechBeatsxGames](https://www.youtube.com/@TechBeatsxGames) | `UCxR_9dxq1OeOCJqW2-q-78A` |

Source playlists:

- Undisputed Bouts: `PLLhVwQz29NhE` — one knockout highlight per fight, branded for esportsboxingleague.com
- Underdogs 2KCL Games NBA2k27 Season 1: `PLUgImi6WUKkE` — one full-game highlight, plus one folder for every player on both teams

The watcher accepts videos from the GameAthlete channel id only, and the YouTube uploader accepts a login for `@TechBeatsxGames` only.

## What a new video becomes

Undisputed bout:

- One knockout highlight, about a minute or less
- The cut starts before the knockout graphic and ends after it
- No commentary voice. Fights with no knockout graphic are skipped
- A bout already clipped the old way is redone as this knockout highlight

The first run marks videos already in the playlists as seen. Boxing videos from that first pass are still redone. 2K videos from that first pass stay as they are until you run `backfill`.

2KCL game:

- One full-game highlight, about 55 seconds
- One highlight folder per player, under `players`, for both teams. Each reel follows that player's points, rebounds, assists, steals, blocks, and dunks when the game shows them. The centered mic is ducked. Crowd and game sound stay. No narrator.
- One UNCS coaching film of the buckets they gave up, plus a screenshot of each of those possessions. The voice talks like a coach: get back, stay down on closeouts, and box out.
- Three league marketing spots with no narrator: a question hook, the closing run, and a 2K Comp League bumper. On-screen type carries the post, then 2kcompleague.com.

Every gamertag read from the 2K player card gets a folder, and so does every name under `#roster`. A player with no tracked stat line still gets that folder. The full highlight stays the loudest stretches of the game, separate from the player folders.

2K captions say the voice is AI-generated. Every caption credits GameAthlete.

## Setup

From `Tools/ChannelAuto`:

```powershell
python -m pip install -r requirements.txt
python -m unittest discover -s tests -t .
python -m channel_auto preview
python -m channel_auto playlists
```

`preview` prints sample titles without downloading. `playlists` shows what is in the two GameAthlete playlists right now.

Leave the watcher running:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_watch.ps1
```

The first run marks videos already in the playlists as seen and does not download them. After that, a video added to either playlist is clipped on the next check (every 10 minutes). Boxing videos that were already cut with commentary are replaced by a knockout highlight on that same check.

To clip the videos that are already there:

```powershell
python -m channel_auto backfill
```

One video:

```powershell
python -m channel_auto process --video VIDEO_ID
```

Finished files:

```text
C:\Users\Shadow\Videos\ready for upload\<game title>\<date>_<id>_full.mp4
C:\Users\Shadow\Videos\ready for upload\<game title>\<date>_<id>_full.jpg
C:\Users\Shadow\Videos\ready for upload\<game title>\<date>_<id>_full.tiktok.txt
```

The `.jpg` is a 1920×1080 featured image. League shorts show the 2K Comp League logo, the name 2K COMP LEAGUE, and 2kcompleague.com. Post them yourself when you are ready. Upload from this folder is not automatic.

Uploads default to **unlisted**, so a bad cut does not go public. In `config/channel.json`, `youtube_privacy` can be `public` after you trust the cuts. A clip with a weak auto-cut stays unlisted even if that setting is `public`.

## Player names

Copy `examples/moments.example.txt` to `inbox/moments/VIDEO_ID.txt`:

```text
#roster
OGSportsGamer
SmokinSpliffss

#moments
0:42 | OGSportsGamer | three | corner three
1:15 | SmokinSpliffss | dunk | putback
```

Or install the ticker reader and let it sample the scorebug during loud plays:

```powershell
python -m pip install -r requirements-ocr.txt
```

That reader is optional. Without it, and without a moments file, you still get the full-game highlight and the boxing commentary.

Upload is off. The watcher only writes files into `ready for upload`.

`secrets/`, `inbox/`, `outbox/`, and `state/` stay off git.
