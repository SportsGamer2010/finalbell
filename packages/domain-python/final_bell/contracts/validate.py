"""Schema validation for shared Final Bell contracts."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator

CONTRACTS_ROOT = Path(__file__).resolve().parents[3] / "contracts"
SCHEMAS = CONTRACTS_ROOT / "schemas"
FIXTURES = CONTRACTS_ROOT / "fixtures"

SCHEMA_FILES = {
    "FighterTrueState": "fighter_true_state.schema.json",
    "FighterReadiness": "fighter_readiness.schema.json",
    "CombatIntent": "combat_intent.schema.json",
    "FightResult": "fight_result.schema.json",
}


def load_schema(name: str) -> dict[str, Any]:
    path = SCHEMAS / SCHEMA_FILES[name]
    with path.open(encoding="utf-8") as fh:
        return json.load(fh)


def validate_payload(name: str, payload: dict[str, Any]) -> None:
    schema = load_schema(name)
    Draft202012Validator(schema).validate(payload)


def load_fixture(filename: str) -> dict[str, Any]:
    with (FIXTURES / filename).open(encoding="utf-8") as fh:
        return json.load(fh)
