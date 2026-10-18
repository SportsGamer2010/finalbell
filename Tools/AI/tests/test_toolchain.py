"""Local tests for routing, fighter specs, and write guards."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from fb_auto.paths import REPO_ROOT  # noqa: E402
from fb_auto.router import extract_args, route  # noqa: E402
from fb_auto.safety import assert_safe_output  # noqa: E402
from validators.animation_names import validate_clip_name  # noqa: E402
from validators.fighter_spec import validate_fighter  # noqa: E402
from workflows.create_boxer import build_fighter  # noqa: E402


class RouterTests(unittest.TestCase):
    def test_examples_from_the_setup_guide(self) -> None:
        expectations = {
            "Create a lightweight boxer with fast hands.": "create-boxer",
            "Add realistic counterpunching.": "gameplay",
            "Improve character rotation and footwork.": "gameplay",
            "Generate a career-mode menu.": "ui-spec",
            "Add facial damage after punches.": "textures",
            "Create a realistic knockout replay.": "animation",
        }
        for request, workflow in expectations.items():
            self.assertEqual(route(request)["primary"], workflow, request)

    def test_three_heavyweights_are_extracted(self) -> None:
        extracted = extract_args(
            "Create three realistic heavyweight fighters with distinct builds, faces, skin tones, and boxing styles."
        )
        self.assertEqual(extracted["weight"], "heavyweight")
        self.assertEqual(extracted["count"], 3)

    def test_route_does_not_claim_execution(self) -> None:
        self.assertFalse(route("Create a lightweight boxer with fast hands.")["executed"])


class FighterTests(unittest.TestCase):
    def test_three_heavyweights_are_distinct_and_valid(self) -> None:
        specs = [build_fighter("heavyweight", "boxer-puncher", 21, index) for index in range(3)]
        heights = {spec["presentation"]["height_cm"] for spec in specs}
        skins = {spec["presentation"]["skin"]["melanin"] for spec in specs}
        self.assertGreaterEqual(len(heights), 3)
        self.assertGreaterEqual(len(skins), 3)
        for spec in specs:
            self.assertEqual(validate_fighter(spec), [])
            self.assertFalse(spec["gameplay"]["hitboxes_follow_appearance"])
            self.assertEqual(spec["sources"]["metahuman"], "not_generated")

    def test_fast_hands_lightweight(self) -> None:
        spec = build_fighter("lightweight", "fast-hands", 11, 0)
        self.assertEqual(validate_fighter(spec), [])
        self.assertGreater(spec["gameplay"]["hand_speed"], spec["gameplay"]["power"])

    def test_rejects_hitboxes_that_follow_appearance(self) -> None:
        spec = build_fighter("middleweight", "out-boxer", 4, 0)
        spec["gameplay"]["hitboxes_follow_appearance"] = True
        self.assertTrue(validate_fighter(spec))


class SafetyTests(unittest.TestCase):
    def test_content_and_source_are_protected(self) -> None:
        for relative in (
            "apps/game-unreal/Content/Generated/nope.json",
            "apps/game-unreal/Source/FinalBell/Nope.cpp",
        ):
            with self.assertRaises(RuntimeError):
                assert_safe_output(REPO_ROOT / relative)

    def test_animation_names(self) -> None:
        self.assertEqual(validate_clip_name("FB_Shared_Punch_Jab_01"), [])
        self.assertTrue(validate_clip_name("jab attack"))


if __name__ == "__main__":
    unittest.main()
