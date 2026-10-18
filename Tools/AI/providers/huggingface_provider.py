"""Search the public Hugging Face model API. This never downloads weights."""

from __future__ import annotations

import json
from urllib.error import URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from fb_auto.logging_util import log_event
from fb_auto.paths import ensure_output
from fb_auto.safety import assert_safe_output


def search_models(query: str, limit: int = 5) -> dict:
    params = urlencode(
        {
            "search": query,
            "limit": str(limit),
            "sort": "downloads",
            "full": "false",
        }
    )
    request = Request(
        f"https://huggingface.co/api/models?{params}",
        headers={"User-Agent": "final-bell-toolchain"},
    )
    try:
        with urlopen(request, timeout=20) as response:
            payload = json.loads(response.read().decode("utf-8"))
            status = response.status
    except URLError as exc:
        result = {"status": "failed", "executed": False, "error": str(exc), "downloaded": False}
        log_event("huggingface", "failed", {"error": str(exc)})
        return result
    models = []
    for item in payload if isinstance(payload, list) else []:
        models.append(
            {
                "id": item.get("id"),
                "pipeline_tag": item.get("pipeline_tag"),
                "license": (item.get("cardData") or {}).get("license") if isinstance(item.get("cardData"), dict) else None,
                "downloads": item.get("downloads"),
                "library": item.get("library_name"),
            }
        )
    result = {
        "status": "completed",
        "executed": True,
        "http_status": status,
        "downloaded": False,
        "query": query,
        "models": models,
        "manual": "Read each model license before commercial use. Do not copy weights into the game repo.",
    }
    destination = assert_safe_output(ensure_output("models") / "last_search.json")
    destination.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    log_event("huggingface", "completed", {"query": query, "count": len(models), "http_status": status})
    result["path"] = str(destination)
    return result
