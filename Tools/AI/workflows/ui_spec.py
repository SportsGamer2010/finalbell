"""Write a UMG layout specification. This does not create a Widget Blueprint."""

from __future__ import annotations

import json

from fb_auto.logging_util import log_event
from fb_auto.paths import CONFIG, ensure_output
from fb_auto.safety import assert_safe_output


def build_ui_spec(screen: str) -> dict:
    screens = json.loads((CONFIG / "ui_screens.json").read_text(encoding="utf-8"))["screens"]
    if screen not in screens:
        known = ", ".join(sorted(screens))
        raise ValueError(f"Unknown screen '{screen}'. Known screens: {known}")
    design = json.loads((CONFIG / "design_system.json").read_text(encoding="utf-8"))
    spec = {
        "status": "completed",
        "executed": True,
        "screen": screen,
        "source": "local-specification",
        "figma_imported": False,
        "umg_asset_created": False,
        "design_system": design,
        "regions": [
            {"name": region, "widget": "Text or Button", "padding": design["spacing"][3]}
            for region in screens[screen]
        ],
        "manual": "Build the UMG widget in the editor from this spec. Figma is not linked.",
    }
    destination = assert_safe_output(ensure_output("ui") / f"{screen}.layout.json")
    destination.write_text(json.dumps(spec, indent=2) + "\n", encoding="utf-8")
    log_event("ui-spec", "completed", {"screen": screen, "path": str(destination)})
    spec["path"] = str(destination)
    return spec
