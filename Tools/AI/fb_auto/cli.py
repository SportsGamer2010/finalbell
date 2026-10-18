"""Command line for the Final Bell automation orchestrator."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from fb_auto.environment import probe
from fb_auto.logging_util import log_event
from fb_auto.paths import LOGS, REPO_ROOT, UPROJECT
from fb_auto.router import extract_args, route
from fb_auto.safety import assert_safe_output
from providers.figma_provider import status as figma_status
from providers.huggingface_provider import search_models
from providers.linear_provider import plan as linear_plan
from validators.animation_names import validate_clip_name
from validators.fighter_spec import validate_file
from workflows.animation_pipeline import build_manifest
from workflows.create_boxer import write_fighters
from workflows.gameplay_guidance import guide
from workflows.texture_pipeline import build_texture_manifest
from workflows.ui_spec import build_ui_spec


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Final Bell local tool router")
    sub = parser.add_subparsers(dest="command", required=True)

    status = sub.add_parser("status")
    status.add_argument("--json", action="store_true")

    route_cmd = sub.add_parser("route")
    route_cmd.add_argument("request")

    run = sub.add_parser("run")
    run.add_argument(
        "workflow",
        choices=(
            "create-boxer",
            "animation-manifest",
            "texture-manifest",
            "ui-spec",
            "gameplay",
            "huggingface-search",
            "linear-plan",
        ),
    )
    run.add_argument("--request", default="")
    run.add_argument("--weight", choices=("lightweight", "middleweight", "heavyweight"))
    run.add_argument("--style", default="boxer-puncher")
    run.add_argument("--count", type=int, default=1)
    run.add_argument("--seed", type=int, default=7)
    run.add_argument("--screen", default="")
    run.add_argument("--query", default="pose estimation")
    run.add_argument("--blender", action="store_true")

    check = sub.add_parser("check")
    check.add_argument("--skip-external", action="store_true")
    check.add_argument("--write-report", action="store_true")

    args = parser.parse_args(argv)
    if args.command == "status":
        payload = probe()
        payload["figma"] = figma_status()
        payload["linear"] = linear_plan()
        _print(payload if args.json else _status_text(payload))
        return 0
    if args.command == "route":
        _print(route(args.request))
        return 0
    if args.command == "run":
        _print(_run(args))
        return 0
    report = _check(skip_external=args.skip_external)
    if args.write_report:
        _write_report(report)
    _print(report)
    return 0 if report["ok"] else 1


def _run(args: argparse.Namespace) -> dict:
    extracted = extract_args(args.request) if args.request else {}
    if args.workflow == "create-boxer":
        weight = args.weight or extracted.get("weight")
        if not weight:
            raise SystemExit("create-boxer needs --weight or a request that names a weight class")
        style = extracted.get("style") or args.style
        count = extracted.get("count") if args.request and not args.weight else args.count
        if args.request and args.count == 1 and extracted.get("count"):
            count = extracted["count"]
        return write_fighters(weight, style, count, args.seed, args.blender)
    if args.workflow == "animation-manifest":
        return build_manifest()
    if args.workflow == "texture-manifest":
        return build_texture_manifest()
    if args.workflow == "ui-spec":
        screen = args.screen or extracted.get("screen") or "main-menu"
        return build_ui_spec(screen)
    if args.workflow == "gameplay":
        return guide(args.request or "gameplay")
    if args.workflow == "huggingface-search":
        return search_models(args.query)
    if args.workflow == "linear-plan":
        return linear_plan()
    raise SystemExit(f"Unknown workflow {args.workflow}")


def _check(skip_external: bool) -> dict:
    environment = probe()
    sample = write_fighters("lightweight", "fast-hands", 1, 11, False)
    spec_path = Path(sample["files"][0])
    spec_errors = validate_file(spec_path)
    name_ok = validate_clip_name("FB_Shared_Punch_Jab_01") == []
    name_bad = validate_clip_name("jab attack") != []
    project_ok = UPROJECT.is_file() and json.loads(UPROJECT.read_text(encoding="utf-8")).get(
        "EngineAssociation"
    ) == "5.8"
    routes = {
        "boxer": route("Create a lightweight boxer with fast hands.")["primary"],
        "counter": route("Add realistic counterpunching.")["primary"],
        "rotation": route("Improve character rotation and footwork.")["primary"],
        "menu": route("Generate a career-mode menu.")["primary"],
        "damage": route("Add facial damage after punches.")["primary"],
        "replay": route("Create a realistic knockout replay.")["primary"],
    }
    expected = {
        "boxer": "create-boxer",
        "counter": "gameplay",
        "rotation": "gameplay",
        "menu": "ui-spec",
        "damage": "textures",
        "replay": "animation",
    }
    route_ok = routes == expected
    invariant_ok = project_ok and not spec_errors and name_ok and name_bad and route_ok
    external = {"skipped": skip_external}
    if not skip_external:
        external = {
            "git": environment["tools"]["git"]["installed"],
            "python": environment["tools"]["python"]["installed"],
            "unreal_editor": environment["tools"]["unreal"]["installed"],
            "msvc": environment["tools"]["msvc"]["installed"],
            "github_cli": environment["tools"]["github_cli"],
            "blender": environment["tools"]["blender"]["installed"],
            "ffmpeg": environment["tools"]["ffmpeg"]["installed"],
            "node": environment["tools"]["node"]["installed"],
        }
    report = {
        "ok": invariant_ok,
        "project_ok": project_ok,
        "sample_fighter": str(spec_path),
        "sample_errors": spec_errors,
        "animation_name_ok": name_ok and name_bad,
        "routes": routes,
        "route_ok": route_ok,
        "environment": environment,
        "external": external,
        "claims": {
            "unreal_build_ran": False,
            "gameplay_playtested": False,
            "metahuman_created": False,
            "figma_imported": False,
            "linear_issues_created": False,
            "models_downloaded": False,
        },
    }
    log_event("check", "completed" if invariant_ok else "failed", {"ok": invariant_ok})
    return report


def _write_report(report: dict) -> None:
    tools = report["environment"]["tools"]
    lines = [
        "# Final Bell integration test report",
        "",
        "Generated by `python Tools/AI/scripts/fb_orchestrator.py check --write-report`.",
        "Figures below are the values returned by that run.",
        "",
        f"- Repo invariants passed: {report['ok']}",
        f"- Unreal project file found and associated with 5.8: {report['project_ok']}",
        f"- Sample fighter: `{report['sample_fighter']}`",
        f"- Sample fighter errors: {report['sample_errors'] or 'none'}",
        f"- Animation name checks passed: {report['animation_name_ok']}",
        f"- Route checks passed: {report['route_ok']}",
        f"- Routes: `{json.dumps(report['routes'])}`",
        "",
        "## Tools observed",
        "",
        f"- Git: {tools['git']}",
        f"- GitHub CLI: {tools['github_cli']}",
        f"- Python: {tools['python']}",
        f"- Blender: {tools['blender']}",
        f"- FFmpeg: {tools['ffmpeg']}",
        f"- Node: {tools['node']}",
        f"- Unreal: {tools['unreal']}",
        f"- MSVC: {tools['msvc']}",
        "",
        "## Not claimed",
        "",
        "- No Unreal editor build was run by this check.",
        "- No gameplay session was playtested.",
        "- No MetaHuman was created.",
        "- No Figma file was imported.",
        "- No Linear issue was created.",
        "- No model weights were downloaded.",
        "",
    ]
    destination = REPO_ROOT / "docs" / "AI_Automation" / "Integration_Test_Report.md"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text("\n".join(lines), encoding="utf-8")
    LOGS.mkdir(parents=True, exist_ok=True)
    raw = assert_safe_output(LOGS / "integration-check.json")
    raw.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def _status_text(payload: dict) -> dict:
    return payload


def _print(payload: dict) -> None:
    json.dump(payload, sys.stdout, indent=2)
    sys.stdout.write("\n")
