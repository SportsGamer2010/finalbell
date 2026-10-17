"""Golden contract checks for FB-002 (Developer B)."""

from __future__ import annotations

import pytest
from jsonschema.exceptions import ValidationError

from final_bell.contracts.validate import load_fixture, validate_payload


@pytest.mark.parametrize(
    "schema_name,fixture",
    [
        ("FighterTrueState", "fighter_true_state.valid.json"),
        ("FighterReadiness", "fighter_readiness.valid.json"),
        ("CombatIntent", "combat_intent.valid.json"),
        ("FightResult", "fight_result.valid.json"),
    ],
)
def test_valid_fixtures_pass(schema_name: str, fixture: str) -> None:
    validate_payload(schema_name, load_fixture(fixture))


def test_bad_schema_version_rejected() -> None:
    with pytest.raises(ValidationError):
        validate_payload("CombatIntent", load_fixture("combat_intent.bad_schema.json"))


def test_wrong_domain_rejected() -> None:
    payload = load_fixture("fighter_true_state.valid.json")
    payload["domain_id"] = "CONSOLE"
    with pytest.raises(ValidationError):
        validate_payload("FighterTrueState", payload)
