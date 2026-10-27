from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import layout as layout_resolver  # noqa: E402
import validate_structure as structure_validator  # noqa: E402


DEFAULT_ANCHOR = {"horizontal": "left", "vertical": "top"}


def add_default_anchors(element: dict) -> None:
    element.setdefault("anchor", dict(DEFAULT_ANCHOR))
    for child in element.get("children") or []:
        if isinstance(child, dict):
            add_default_anchors(child)


def box(name: str, width: int, height: int) -> dict:
    return {
        "type": "rect",
        "name": name,
        "size": {"width": width, "height": height},
        "anchor": dict(DEFAULT_ANCHOR),
        "color": "#FFFFFF",
    }


class LayoutDistributionTests(unittest.TestCase):
    def resolve(
        self,
        *,
        layout_type: str,
        align: str,
        parent_main: int,
        child_main: int,
        child_count: int,
    ) -> dict:
        if layout_type == "row":
            parent_size = {"width": parent_main, "height": 40}
            children = [box(f"child_{i}", child_main, 10) for i in range(child_count)]
        else:
            parent_size = {"width": 40, "height": parent_main}
            children = [box(f"child_{i}", 10, child_main) for i in range(child_count)]

        structure = {
            "canvas": {"width": 200, "height": 200},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": parent_size,
                "layout": {
                    "type": layout_type,
                    "align": align,
                    "vAlign": "middle",
                },
                "children": children,
            },
        }
        return layout_resolver.resolve_positions(structure)["root"]

    def main_axis_gaps(self, root: dict, layout_type: str) -> list[int]:
        coordinate = 0 if layout_type == "row" else 1
        extent = 2 if layout_type == "row" else 3
        children = root["children"]
        gaps = [children[0]["_rel"][coordinate]]
        gaps.extend(
            following["_rel"][coordinate]
            - (preceding["_rel"][coordinate] + preceding["_rel"][extent])
            for preceding, following in zip(children, children[1:])
        )
        gaps.append(
            root["_rel"][extent]
            - (children[-1]["_rel"][coordinate] + children[-1]["_rel"][extent])
        )
        return gaps

    def test_single_space_around_and_space_evenly_are_centered(self) -> None:
        for layout_type in ("row", "column"):
            for align in ("space-around", "space-evenly"):
                with self.subTest(layout_type=layout_type, align=align):
                    root = self.resolve(
                        layout_type=layout_type,
                        align=align,
                        parent_main=21,
                        child_main=10,
                        child_count=1,
                    )
                    self.assertEqual(self.main_axis_gaps(root, layout_type), [5, 6])

    def test_odd_remainders_are_distributed_without_end_drift(self) -> None:
        cases = (
            ("row", "space-evenly", 41, 3, [2, 3, 3, 3]),
            ("column", "space-between", 45, 3, [0, 7, 8, 0]),
            ("row", "space-around", 117, 4, [9, 19, 20, 19, 10]),
        )
        for layout_type, align, parent_main, child_count, expected in cases:
            with self.subTest(layout_type=layout_type, align=align):
                root = self.resolve(
                    layout_type=layout_type,
                    align=align,
                    parent_main=parent_main,
                    child_main=10,
                    child_count=child_count,
                )
                gaps = self.main_axis_gaps(root, layout_type)
                self.assertEqual(gaps, expected)
                self.assertLessEqual(abs(gaps[0] - gaps[-1]), 1)


class StructureValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.design = self.root / "design.png"
        self.assets = self.root / "assets"
        self.assets.mkdir()
        Image.new("RGB", (64, 64), "black").save(self.design)
        Image.new("RGBA", (20, 10), "white").save(self.assets / "panel.png")

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def validate_with_stats(
        self, root: dict
    ) -> tuple[structure_validator.Reporter, dict[str, int]]:
        add_default_anchors(root)
        return structure_validator.validate_structure(
            {
                "canvas": {"width": 64, "height": 64},
                "root": root,
            },
            self.design,
            self.assets,
        )

    def validate(self, root: dict) -> structure_validator.Reporter:
        reporter, _stats = self.validate_with_stats(root)
        return reporter

    def validate_layout(self, layout: object) -> structure_validator.Reporter:
        return self.validate(
            {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "layout": layout,
                "children": [box("probe", 10, 10)],
            }
        )

    def validate_nine_slice(self, value: object) -> structure_validator.Reporter:
        return self.validate(
            {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": [
                    {
                        "type": "image",
                        "name": "panel",
                        "position": {"x": 0, "y": 0},
                        "size": {"width": 40, "height": 20},
                        "asset": "panel.png",
                        "nineSlice": value,
                    }
                ],
            }
        )

    def validate_child(self, child: dict) -> structure_validator.Reporter:
        return self.validate(
            {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": [child],
            }
        )

    def test_element_name_must_be_a_stable_path_segment(self) -> None:
        for name in ("menu/item", "menu\\item", ".", ".."):
            with self.subTest(name=name):
                child = box(name, 10, 10)
                child["position"] = {"x": 0, "y": 0}

                reporter = self.validate_child(child)

                self.assertTrue(
                    any(
                        "name must be a stable path segment" in error
                        for error in reporter.errors
                    ),
                    reporter.errors,
                )

    def test_element_name_allows_non_separator_punctuation(self) -> None:
        for name in (
            "button.primary",
            "button..secondary",
            "settings icon",
            "cta-button_2",
        ):
            with self.subTest(name=name):
                child = box(name, 10, 10)
                child["position"] = {"x": 0, "y": 0}

                reporter = self.validate_child(child)

                self.assertEqual(reporter.errors, [])

    def test_text_requires_a_non_empty_string(self) -> None:
        cases = (
            {},
            {"text": ""},
            {"text": " \n\t"},
            {"text": None},
            {"text": 7},
        )
        for fields in cases:
            with self.subTest(fields=fields):
                child = {
                    "type": "text",
                    "name": "label",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 20, "height": 10},
                    **fields,
                }

                reporter = self.validate_child(child)

                self.assertTrue(
                    any("text must be a non-empty string" in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_non_empty_text_remains_valid(self) -> None:
        reporter = self.validate_child(
            {
                "type": "text",
                "name": "label",
                "position": {"x": 0, "y": 0},
                "size": {"width": 20, "height": 10},
                "text": "OK",
            }
        )

        self.assertEqual(reporter.errors, [])

    def test_hue_shift_accepts_finite_degrees_in_range(self) -> None:
        for value in (-360, -75.5, 0, 120, 360):
            with self.subTest(value=value):
                reporter = self.validate_child({
                    "type": "image",
                    "name": "gem",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 20, "height": 10},
                    "asset": "panel.png",
                    "hueShift": value,
                })
                self.assertEqual(reporter.errors, [])

    def test_hue_shift_rejects_invalid_values(self) -> None:
        for value in (True, "30", float("nan"), float("inf"), -361, 361):
            with self.subTest(value=value):
                reporter = self.validate_child({
                    "type": "image",
                    "name": "gem",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 20, "height": 10},
                    "asset": "panel.png",
                    "hueShift": value,
                })
                self.assertTrue(
                    any("hueShift must be" in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_non_root_leaf_container_and_button_require_visual_content(self) -> None:
        for element_type in ("container", "button"):
            for children in (None, []):
                with self.subTest(element_type=element_type, children=children):
                    child = {
                        "type": element_type,
                        "name": "empty_leaf",
                        "position": {"x": 0, "y": 0},
                        "size": {"width": 20, "height": 10},
                    }
                    if children is not None:
                        child["children"] = children

                    reporter = self.validate_child(child)

                    self.assertTrue(
                        any(
                            "must have children or its own visual" in error
                            for error in reporter.errors
                        ),
                        reporter.errors,
                    )

    def test_root_and_visual_or_composite_leaves_remain_valid(self) -> None:
        roots_and_children = (
            {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": [],
            },
            {
                "type": "container",
                "name": "visual_container",
                "position": {"x": 0, "y": 0},
                "size": {"width": 20, "height": 10},
                "color": "#FFFFFF",
            },
            {
                "type": "button",
                "name": "visual_button",
                "position": {"x": 0, "y": 0},
                "size": {"width": 20, "height": 10},
                "asset": "panel.png",
            },
            {
                "type": "button",
                "name": "composite_button",
                "position": {"x": 0, "y": 0},
                "size": {"width": 20, "height": 10},
                "children": [box("face", 20, 10)],
            },
        )
        for element in roots_and_children:
            with self.subTest(name=element["name"]):
                reporter = (
                    self.validate(element)
                    if element["name"] == "root"
                    else self.validate_child(element)
                )
                self.assertFalse(
                    any(
                        "must have children or its own visual" in error
                        for error in reporter.errors
                    ),
                    reporter.errors,
                )

    def test_layout_requires_an_explicit_type(self) -> None:
        for layout in ({}, {"spacing": 5}, {"align": "center"}):
            with self.subTest(layout=layout):
                reporter = self.validate_layout(layout)

                self.assertTrue(
                    any("layout.type is required" in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_layout_rejects_invalid_explicit_types(self) -> None:
        for layout_type in (None, "diagonal", [], True):
            with self.subTest(layout_type=layout_type):
                reporter = self.validate_layout({"type": layout_type})

                self.assertTrue(
                    any(
                        "layout.type must be row or column" in error
                        for error in reporter.errors
                    ),
                    reporter.errors,
                )

    def test_resolver_rejects_layouts_without_an_explicit_type(self) -> None:
        invalid_layouts = (
            {},
            {"spacing": 5},
            None,
            False,
            {"type": None},
            {"type": "diagonal"},
            {"type": []},
            {"type": True},
        )
        for layout in invalid_layouts:
            with self.subTest(layout=layout):
                structure = {
                    "canvas": {"width": 64, "height": 64},
                    "root": {
                        "type": "container",
                        "name": "root",
                        "position": {"x": 0, "y": 0},
                        "size": {"width": 64, "height": 64},
                        "layout": layout,
                        "children": [box("probe", 10, 10)],
                    },
                }

                with self.assertRaisesRegex(ValueError, "layout.type"):
                    layout_resolver.resolve_positions(structure)

    def test_layout_enums_reject_values_for_the_wrong_axis(self) -> None:
        cases = (
            ("row", "align", "top"),
            ("row", "vAlign", "left"),
            ("row", "vAlign", "space-around"),
            ("column", "align", "right"),
            ("column", "vAlign", "bottom"),
            ("column", "vAlign", "space-evenly"),
        )
        for layout_type, field, value in cases:
            with self.subTest(layout_type=layout_type, field=field, value=value):
                reporter = self.validate_layout(
                    {"type": layout_type, field: value}
                )
                self.assertTrue(
                    any(f"layout.{field}" in error for error in reporter.errors),
                    reporter.errors,
                )
                self.assertFalse(
                    any(f"layout.{field}" in warning for warning in reporter.warnings),
                    reporter.warnings,
                )

    def test_meaningful_axis_aliases_remain_valid(self) -> None:
        layouts = (
            {"type": "row", "align": "right", "vAlign": "top"},
            {"type": "column", "align": "bottom", "vAlign": "left"},
            {"type": "row", "align": "space-between", "vAlign": "middle"},
            {"type": "column", "align": "space-around", "vAlign": "center"},
        )
        for layout in layouts:
            with self.subTest(layout=layout):
                self.assertEqual(self.validate_layout(layout).errors, [])

    def test_evenly_spaced_fixed_slots_do_not_require_list_roles(self) -> None:
        reporter, stats = self.validate_with_stats({
            "type": "container",
            "name": "root",
            "position": {"x": 0, "y": 0},
            "size": {"width": 64, "height": 64},
            "layout": {
                "type": "row",
                "spacing": "even",
                "align": "space-evenly",
                "vAlign": "middle",
            },
            "children": [
                box("slot_1", 10, 10),
                box("slot_2", 10, 10),
                box("slot_3", 10, 10),
            ],
        })

        self.assertEqual(reporter.errors, [])
        self.assertEqual(stats["lists"], 0)
        self.assertEqual(stats["list_items"], 0)

    def test_list_role_requires_a_laid_out_container_with_items(self) -> None:
        list_node = {
            "type": "container",
            "name": "quest_list",
            "position": {"x": 0, "y": 0},
            "size": {"width": 40, "height": 40},
            "role": "list",
            "layout": {"type": "column", "spacing": 4},
            "children": [
                {
                    **box("quest", 20, 10),
                    "role": "listItem",
                }
            ],
        }

        reporter = self.validate_child(list_node)

        self.assertEqual(reporter.errors, [])

    def test_role_rejects_unknown_values(self) -> None:
        child = box("probe", 10, 10)
        child.update({"position": {"x": 0, "y": 0}, "role": "collection"})

        reporter = self.validate_child(child)

        self.assertTrue(
            any("role must be list or listItem" in error for error in reporter.errors),
            reporter.errors,
        )

    def test_list_role_requires_container_layout_and_children(self) -> None:
        cases = (
            (
                {
                    **box("not_container", 20, 20),
                    "position": {"x": 0, "y": 0},
                    "role": "list",
                    "layout": {"type": "column"},
                },
                "role list is only valid on container elements",
            ),
            (
                {
                    "type": "container",
                    "name": "missing_layout",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 20, "height": 20},
                    "role": "list",
                    "color": "#FFFFFF",
                    "children": [],
                },
                "role list requires a row or column or grid layout",
            ),
            (
                {
                    "type": "container",
                    "name": "missing_items",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 20, "height": 20},
                    "role": "list",
                    "layout": {"type": "column"},
                    "color": "#FFFFFF",
                    "children": [],
                },
                "role list requires at least one listItem child",
            ),
        )
        for child, message in cases:
            with self.subTest(message=message):
                reporter = self.validate_child(child)
                self.assertTrue(
                    any(message in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_list_items_must_be_direct_children_of_a_list(self) -> None:
        orphan = box("orphan", 10, 10)
        orphan.update({
            "position": {"x": 0, "y": 0},
            "role": "listItem",
        })

        reporter = self.validate_child(orphan)

        self.assertTrue(
            any(
                "role listItem requires a direct role list parent" in error
                for error in reporter.errors
            ),
            reporter.errors,
        )

    def test_every_direct_list_child_must_be_a_list_item(self) -> None:
        list_node = {
            "type": "container",
            "name": "quest_list",
            "position": {"x": 0, "y": 0},
            "size": {"width": 40, "height": 40},
            "role": "list",
            "layout": {"type": "column"},
            "children": [box("quest", 20, 10)],
        }

        reporter = self.validate_child(list_node)

        self.assertTrue(
            any(
                "direct children of role list must use role listItem" in error
                for error in reporter.errors
            ),
            reporter.errors,
        )

    def test_asset_relative_paths_cannot_collide_after_casefold(self) -> None:
        first = self.assets / "stra\u00dfe.png"
        second = self.assets / "STRASSE.PNG"
        first.write_bytes(b"first")
        second.write_bytes(b"second")
        if len([path for path in self.assets.iterdir() if path.stem.casefold() == "strasse"]) < 2:
            self.skipTest("filesystem does not preserve the case-fold-distinct fixture names")

        reporter = self.validate(
            {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": [],
            }
        )

        self.assertTrue(
            any("case-fold" in error and "relative path" in error for error in reporter.errors),
            reporter.errors,
        )

    def test_nine_slice_rejects_non_finite_and_negative_scalars(self) -> None:
        for value in (-1, float("nan"), float("inf"), float("-inf")):
            with self.subTest(value=value):
                reporter = self.validate_nine_slice(value)
                self.assertTrue(
                    any("nineSlice" in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_nine_slice_rejects_invalid_margin_objects(self) -> None:
        cases = (
            ({"left": -1}, "nineSlice.left"),
            ({"top": float("inf")}, "nineSlice.top"),
            ({"center": 2}, "unknown margin"),
            ({"left": 11, "right": 10}, "horizontal margins"),
            ({"top": 6, "bottom": 5}, "vertical margins"),
        )
        for value, message in cases:
            with self.subTest(value=value):
                reporter = self.validate_nine_slice(value)
                self.assertTrue(
                    any(message in error for error in reporter.errors),
                    reporter.errors,
                )

    def test_nine_slice_accepts_finite_legal_margins(self) -> None:
        values = (
            False,
            True,
            "auto",
            "meta",
            0,
            5,
            2.5,
            {},
            {"left": 20},
            {"top": 4, "bottom": 6},
        )
        for value in values:
            with self.subTest(value=value):
                self.assertEqual(self.validate_nine_slice(value).errors, [])

    def test_numeric_one_is_not_treated_as_boolean_auto(self) -> None:
        for value in (1, 1.0):
            with self.subTest(value=value):
                reporter = self.validate_nine_slice(value)
                self.assertFalse(
                    any("spriteBorder metadata" in warning for warning in reporter.warnings),
                    reporter.warnings,
                )

    def test_nine_slice_validates_unity_metadata_margins(self) -> None:
        for left in ("-1", "nan", "inf"):
            with self.subTest(left=left):
                (self.assets / "panel.png.meta").write_text(
                    f"spriteBorder: {{x: {left}, y: 0, z: 0, w: 0}}\n",
                    encoding="utf-8",
                )

                reporter = self.validate_nine_slice("meta")

                self.assertTrue(
                    any("nineSlice.left" in error for error in reporter.errors),
                    reporter.errors,
                )


if __name__ == "__main__":
    unittest.main()
