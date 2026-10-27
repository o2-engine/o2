from __future__ import annotations

import copy
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import backfill_anchors  # noqa: E402
import validate_structure as structure_validator  # noqa: E402


LEFT_TOP = {"horizontal": "left", "vertical": "top"}


def rect(
    name: str,
    position: tuple[int, int] | None = None,
    *,
    anchor: dict[str, str] | None = None,
) -> dict:
    element = {
        "type": "rect",
        "name": name,
        "size": {"width": 20, "height": 20},
        "color": "#FFFFFF",
    }
    if position is not None:
        element["position"] = {"x": position[0], "y": position[1]}
    if anchor is not None:
        element["anchor"] = anchor
    return element


class AnchorValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.workspace = Path(self.temporary.name)
        self.design = self.workspace / "design.png"
        self.assets = self.workspace / "assets"
        self.assets.mkdir()
        Image.new("RGB", (100, 100), "black").save(self.design)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def structure(self) -> dict:
        return {
            "canvas": {"width": 100, "height": 100},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 100, "height": 100},
                "anchor": dict(LEFT_TOP),
                "children": [rect("probe", (10, 10), anchor=dict(LEFT_TOP))],
            },
        }

    def validate(self, structure: dict) -> structure_validator.Reporter:
        reporter, _stats = structure_validator.validate_structure(
            structure,
            self.design,
            self.assets,
        )
        return reporter

    def test_every_node_requires_an_anchor(self) -> None:
        for missing_path in ("root", "root/probe"):
            with self.subTest(path=missing_path):
                structure = self.structure()
                target = (
                    structure["root"]
                    if missing_path == "root"
                    else structure["root"]["children"][0]
                )
                del target["anchor"]

                reporter = self.validate(structure)

                self.assertTrue(
                    any(
                        error == f"{missing_path}: anchor is required"
                        for error in reporter.errors
                    ),
                    reporter.errors,
                )

    def test_anchor_requires_both_canonical_axes(self) -> None:
        invalid = (
            (None, "anchor must be an object"),
            ({}, "anchor.horizontal is required"),
            ({"horizontal": "left"}, "anchor.vertical is required"),
            (
                {"horizontal": "start", "vertical": "top"},
                "anchor.horizontal must be one of",
            ),
            (
                {"horizontal": "left", "vertical": "center"},
                "anchor.vertical must be one of",
            ),
            (
                {"horizontal": "left", "vertical": "top", "x": 0},
                "anchor has unknown fields",
            ),
        )
        for value, message in invalid:
            with self.subTest(anchor=value):
                structure = self.structure()
                structure["root"]["children"][0]["anchor"] = value

                reporter = self.validate(structure)

                self.assertTrue(
                    any(message in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_all_nine_anchor_combinations_are_valid(self) -> None:
        for horizontal in ("left", "center", "right"):
            for vertical in ("top", "middle", "bottom"):
                with self.subTest(horizontal=horizontal, vertical=vertical):
                    structure = self.structure()
                    structure["root"]["children"][0]["anchor"] = {
                        "horizontal": horizontal,
                        "vertical": vertical,
                    }

                    self.assertEqual(self.validate(structure).errors, [])


class AnchorBackfillTests(unittest.TestCase):
    def test_full_size_artwork_tie_defaults_to_center_without_stretch(self):
        source = {"canvas": {"width": 100, "height": 100}, "root": {
            "type": "container", "name": "root", "size": {"width": 100, "height": 100},
            "children": [{**rect("artwork", (0, 0)), "size": {"width": 100, "height": 100}}]}}
        result, _ = backfill_anchors.backfill_anchors(source)
        artwork = result["root"]["children"][0]
        self.assertEqual(artwork["anchor"], {"horizontal": "center", "vertical": "middle"})
        self.assertNotIn("responsive", artwork)
        self.assertEqual(artwork["size"], {"width": 100, "height": 100})
        self.assertEqual(result["root"]["anchor"], LEFT_TOP)

    def test_backfill_prefers_explicit_alignment_intent(self) -> None:
        source = {
            "canvas": {"width": 100, "height": 100},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 100, "height": 100},
                "children": [{
                    **rect("full_size"),
                    "size": {"width": 100, "height": 100},
                    "align": "center",
                    "vAlign": "middle",
                }],
            },
        }

        result, _stats = backfill_anchors.backfill_anchors(source)

        self.assertEqual(
            result["root"]["children"][0]["anchor"],
            {"horizontal": "center", "vertical": "middle"},
        )

    def test_backfill_uses_resolved_layout_geometry(self) -> None:
        source = {
            "canvas": {"width": 300, "height": 100},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 300, "height": 100},
                "layout": {
                    "type": "row",
                    "align": "space-between",
                    "vAlign": "middle",
                },
                "children": [rect("left"), rect("center"), rect("right")],
            },
        }

        result, stats = backfill_anchors.backfill_anchors(source)

        self.assertNotIn("anchor", source["root"])
        self.assertEqual(result["root"]["anchor"], LEFT_TOP)
        self.assertEqual(
            [child["anchor"] for child in result["root"]["children"]],
            [
                {"horizontal": "left", "vertical": "middle"},
                {"horizontal": "center", "vertical": "middle"},
                {"horizontal": "right", "vertical": "middle"},
            ],
        )
        self.assertEqual(stats, {
            "total": 4,
            "added": 4,
            "updated": 0,
            "unchanged": 0,
        })
        self.assertNotIn("_abs", result["root"])
        self.assertNotIn("_rel", result["root"]["children"][0])

    def test_backfill_preserves_valid_values_and_fills_partial_anchor(self) -> None:
        source = {
            "canvas": {"width": 100, "height": 100},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 100, "height": 100},
                "anchor": {"horizontal": "right", "vertical": "bottom"},
                "children": [
                    rect(
                        "probe",
                        (40, 40),
                        anchor={"horizontal": "center"},
                    )
                ],
            },
        }

        result, stats = backfill_anchors.backfill_anchors(source)

        self.assertEqual(
            result["root"]["anchor"],
            {"horizontal": "right", "vertical": "bottom"},
        )
        self.assertEqual(
            result["root"]["children"][0]["anchor"],
            {"horizontal": "center", "vertical": "middle"},
        )
        self.assertEqual(stats, {
            "total": 2,
            "added": 0,
            "updated": 1,
            "unchanged": 1,
        })

    def test_invalid_existing_anchor_requires_explicit_overwrite(self) -> None:
        source = {
            "canvas": {"width": 100, "height": 100},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 100, "height": 100},
                "anchor": {"horizontal": "near", "vertical": "top"},
                "children": [],
            },
        }

        with self.assertRaisesRegex(
            backfill_anchors.AnchorBackfillError,
            "root.anchor.horizontal",
        ):
            backfill_anchors.backfill_anchors(source)

        result, stats = backfill_anchors.backfill_anchors(
            copy.deepcopy(source),
            overwrite=True,
        )
        self.assertEqual(result["root"]["anchor"], LEFT_TOP)
        self.assertEqual(stats["updated"], 1)


if __name__ == "__main__":
    unittest.main()
