from pathlib import Path

from channel_auto.config import Settings, require_channel

SCOPES = [
    "https://www.googleapis.com/auth/youtube.upload",
    "https://www.googleapis.com/auth/youtube.force-ssl",
    "https://www.googleapis.com/auth/youtube.readonly",
]


def privacy_for(confident: bool, configured: str) -> str:
    if not confident:
        return "unlisted"
    if configured not in {"public", "unlisted", "private"}:
        return "unlisted"
    return configured


def _client_path(settings: Settings) -> Path:
    return settings.secrets / "client_secret.json"


def _token_path(settings: Settings) -> Path:
    return settings.secrets / "youtube_token.json"


def credentials_ready(settings: Settings) -> bool:
    return _client_path(settings).exists()


def build_service(settings: Settings):
    from google.auth.transport.requests import Request
    from google.oauth2.credentials import Credentials
    from google_auth_oauthlib.flow import InstalledAppFlow
    from googleapiclient.discovery import build

    token_path = _token_path(settings)
    creds = None
    if token_path.exists():
        creds = Credentials.from_authorized_user_file(str(token_path), SCOPES)
    if creds is None or not creds.valid:
        if creds is not None and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(str(_client_path(settings)), SCOPES)
            creds = flow.run_local_server(port=0)
        token_path.parent.mkdir(parents=True, exist_ok=True)
        token_path.write_text(creds.to_json(), encoding="utf-8")
    return build("youtube", "v3", credentials=creds)


def authorized_channel_id(service) -> str:
    response = service.channels().list(part="id", mine=True).execute()
    items = response.get("items") or []
    if not items:
        raise RuntimeError("The YouTube login does not have a channel.")
    return items[0]["id"]


def upload_short(
    settings: Settings,
    video: Path,
    title: str,
    description: str,
    *,
    confident: bool,
    thumbnail: Path | None = None,
) -> str:
    if not credentials_ready(settings):
        raise FileNotFoundError(
            "Missing Tools/ChannelAuto/secrets/client_secret.json. "
            "The clip and thumbnail are still in the Videos folder."
        )
    from googleapiclient.errors import HttpError
    from googleapiclient.http import MediaFileUpload

    service = build_service(settings)
    require_channel(
        authorized_channel_id(service),
        settings.destination_channel_id,
        "Upload",
    )
    privacy = privacy_for(confident, settings.youtube_privacy)

    def _insert(synthetic: bool):
        status = {
            "privacyStatus": privacy,
            "selfDeclaredMadeForKids": False,
            "embeddable": True,
        }
        if synthetic:
            status["containsSyntheticMedia"] = True
        body = {
            "snippet": {
                "title": title[:100],
                "description": description[:5000],
                "categoryId": "20",
            },
            "status": status,
        }
        media = MediaFileUpload(str(video), mimetype="video/mp4", resumable=True)
        return (
            service.videos()
            .insert(part="snippet,status", body=body, media_body=media)
            .execute()
        )

    try:
        response = _insert(True)
    except HttpError as exc:
        if "containsSyntheticMedia" not in str(exc):
            raise
        response = _insert(False)
    video_id = response["id"]
    if thumbnail is not None and thumbnail.exists():
        try:
            service.thumbnails().set(
                videoId=video_id,
                media_body=MediaFileUpload(str(thumbnail), mimetype="image/jpeg"),
            ).execute()
        except HttpError as exc:
            print(f"YouTube kept the auto thumbnail. Custom image stays at {thumbnail}: {exc}")
    return f"https://www.youtube.com/watch?v={video_id}"
