import asyncio
from pathlib import Path


def synthesize(text: str, voice: str, dest: Path) -> bool:
    try:
        import edge_tts
    except ImportError:
        print("edge-tts is not installed. The clip will keep the game audio and burned-in captions.")
        return False

    async def _save() -> None:
        communicate = edge_tts.Communicate(text, voice)
        await communicate.save(str(dest))

    dest.parent.mkdir(parents=True, exist_ok=True)
    try:
        asyncio.run(_save())
    except Exception as exc:
        print(f"Voice synthesis failed: {exc}")
        return False
    return dest.exists() and dest.stat().st_size > 0
