from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import layout as layout_resolver  # noqa: E402


def box(name: str, width: int, height: int, **fields: object) -> dict:
    element = {
        "type": "rect",
        "name": name,
        "size": {"width": width, "height": height},
    }
    element.update(fields)
    return element


def without_resolved_fields(value: object) -> object:
    if isinstance(value, dict):
        return {
            key: without_resolved_fields(item)
            for key, item in value.items()
            if key not in {"_abs", "_rel"}
        }
    if isinstance(value, list):
        return [without_resolved_fields(item) for item in value]
    return value


class LayoutPropertyTests(unittest.TestCase):
    def structure(
        self,
        *,
        size: tuple[int, int],
        position: tuple[int, int],
        layout: dict,
        children: list[dict],
    ) -> dict:
        width, height = size
        x, y = position
        return {
            "canvas": {"width": x + width + 20, "height": y + height + 20},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": x, "y": y},
                "size": {"width": width, "height": height},
                "layout": layout,
                "children": children,
            },
        }

    def resolve(self, structure: dict) -> list[dict]:
        resolved = layout_resolver.resolve_positions(structure)
        self.assertIs(resolved, structure)
        self.assert_bbox_relationships(structure["root"])
        return structure["root"]["children"]

    def assert_bbox_relationships(self, root: dict) -> None:
        self.assertEqual(root["_rel"], root["_abs"])

        def visit(parent: dict) -> None:
            parent_x, parent_y, _parent_w, _parent_h = parent["_abs"]
            for child in parent.get("children", []):
                rel_x, rel_y, rel_w, rel_h = child["_rel"]
                abs_x, abs_y, abs_w, abs_h = child["_abs"]
                self.assertEqual((abs_x, abs_y), (parent_x + rel_x, parent_y + rel_y))
                self.assertEqual((abs_w, abs_h), (rel_w, rel_h))
                self.assertEqual(
                    (rel_w, rel_h),
                    (child["size"]["width"], child["size"]["height"]),
                )
                visit(child)

        visit(root)

    def main_axis_gaps(
        self,
        root: dict,
        *,
        axis: str,
        padding: int,
    ) -> list[int]:
        coordinate = 0 if axis == "x" else 1
        extent = 2 if axis == "x" else 3
        parent_extent = root["_rel"][extent]
        bboxes = [child["_rel"] for child in root["children"]]
        gaps = [bboxes[0][coordinate] - padding]
        gaps.extend(
            following[coordinate] - (preceding[coordinate] + preceding[extent])
            for preceding, following in zip(bboxes, bboxes[1:])
        )
        gaps.append(
            parent_extent - padding - (bboxes[-1][coordinate] + bboxes[-1][extent])
        )
        return gaps

    def test_row_centering_balances_group_and_cross_axis(self) -> None:
        structure = self.structure(
            size=(120, 90),
            position=(41, 23),
            layout={
                "type": "row",
                "spacing": 10,
                "padding": {"x": 10, "y": 8},
                "align": "center",
                "vAlign": "middle",
            },
            children=[box("short", 20, 10), box("tall", 30, 20)],
        )

        short, tall = self.resolve(structure)

        self.assertEqual(short["_rel"], [30, 40, 20, 10])
        self.assertEqual(short["_abs"], [71, 63, 20, 10])
        self.assertEqual(tall["_rel"], [60, 35, 30, 20])
        self.assertEqual(tall["_abs"], [101, 58, 30, 20])
        self.assertEqual(
            self.main_axis_gaps(structure["root"], axis="x", padding=10),
            [20, 10, 20],
        )
        for child in (short, tall):
            self.assertEqual(2 * child["_rel"][1] + child["_rel"][3], 90)

    def test_column_centering_balances_group_and_cross_axis(self) -> None:
        structure = self.structure(
            size=(80, 130),
            position=(13, 29),
            layout={
                "type": "column",
                "spacing": 10,
                "padding": {"x": 6, "y": 10},
                "align": "center",
                "vAlign": "middle",
            },
            children=[box("narrow", 10, 20), box("wide", 20, 30)],
        )

        narrow, wide = self.resolve(structure)

        self.assertEqual(narrow["_rel"], [35, 35, 10, 20])
        self.assertEqual(narrow["_abs"], [48, 64, 10, 20])
        self.assertEqual(wide["_rel"], [30, 65, 20, 30])
        self.assertEqual(wide["_abs"], [43, 94, 20, 30])
        self.assertEqual(
            self.main_axis_gaps(structure["root"], axis="y", padding=10),
            [25, 10, 25],
        )
        for child in (narrow, wide):
            self.assertEqual(2 * child["_rel"][0] + child["_rel"][2], 80)

    def test_space_evenly_has_equal_outer_and_inner_gaps(self) -> None:
        structure = self.structure(
            size=(100, 50),
            position=(7, 11),
            layout={
                "type": "row",
                "padding": {"x": 5, "y": 5},
                "align": "space-evenly",
                "vAlign": "middle",
            },
            children=[
                box("small", 10, 8),
                box("medium", 10, 12),
                box("large", 10, 16),
            ],
        )

        children = self.resolve(structure)

        self.assertEqual(
            [child["_rel"] for child in children],
            [[20, 21, 10, 8], [45, 19, 10, 12], [70, 17, 10, 16]],
        )
        gaps = self.main_axis_gaps(structure["root"], axis="x", padding=5)
        self.assertEqual(gaps, [15, 15, 15, 15])
        self.assertEqual(len(set(gaps)), 1)

    def test_space_between_pins_outer_edges_and_equalizes_inner_gaps(self) -> None:
        structure = self.structure(
            size=(60, 100),
            position=(11, 17),
            layout={
                "type": "column",
                "padding": {"x": 5, "y": 10},
                "align": "space-between",
                "vAlign": "middle",
            },
            children=[
                box("top", 10, 10),
                box("middle", 10, 20),
                box("bottom", 10, 10),
            ],
        )

        children = self.resolve(structure)

        self.assertEqual(
            [child["_rel"] for child in children],
            [[25, 10, 10, 10], [25, 40, 10, 20], [25, 80, 10, 10]],
        )
        gaps = self.main_axis_gaps(structure["root"], axis="y", padding=10)
        self.assertEqual(gaps[0], 0)
        self.assertEqual(gaps[-1], 0)
        self.assertEqual(gaps[1:-1], [20, 20])

    def test_odd_centering_remainder_is_split_with_one_pixel_bias_to_end(self) -> None:
        structure = self.structure(
            size=(100, 51),
            position=(13, 17),
            layout={
                "type": "row",
                "spacing": 10,
                "align": "center",
                "vAlign": "middle",
            },
            children=[box("first", 20, 10), box("second", 29, 10)],
        )

        first, second = self.resolve(structure)

        self.assertEqual(first["_rel"], [20, 20, 20, 10])
        self.assertEqual(first["_abs"], [33, 37, 20, 10])
        self.assertEqual(second["_rel"], [50, 20, 29, 10])
        self.assertEqual(second["_abs"], [63, 37, 29, 10])
        leading, interior, trailing = self.main_axis_gaps(
            structure["root"], axis="x", padding=0
        )
        self.assertEqual(interior, 10)
        self.assertEqual(trailing, leading + 1)
        for child in (first, second):
            top = child["_rel"][1]
            bottom = 51 - (top + child["_rel"][3])
            self.assertEqual(bottom, top + 1)

    def test_row_child_vertical_alignment_overrides_only_cross_axis(self) -> None:
        structure = self.structure(
            size=(80, 50),
            position=(20, 30),
            layout={
                "type": "row",
                "spacing": 10,
                "padding": 5,
                "align": "start",
                "vAlign": "middle",
            },
            children=[
                box(
                    "top",
                    10,
                    10,
                    position={"x": 500, "y": 500},
                    align="right",
                    vAlign="top",
                ),
                box("middle", 10, 10, position={"x": 500, "y": 500}),
                box(
                    "bottom",
                    10,
                    10,
                    position={"x": 500, "y": 500},
                    vAlign="bottom",
                ),
            ],
        )

        children = self.resolve(structure)

        self.assertEqual(
            [child["_rel"] for child in children],
            [[5, 5, 10, 10], [25, 20, 10, 10], [45, 35, 10, 10]],
        )
        self.assertEqual([child["_abs"][0] for child in children], [25, 45, 65])

    def test_column_child_horizontal_alignment_overrides_only_cross_axis(self) -> None:
        structure = self.structure(
            size=(60, 80),
            position=(30, 40),
            layout={
                "type": "column",
                "spacing": 10,
                "padding": 5,
                "align": "start",
                "vAlign": "middle",
            },
            children=[
                box(
                    "left",
                    10,
                    10,
                    position={"x": 500, "y": 500},
                    align="left",
                    vAlign="bottom",
                ),
                box("middle", 10, 10, position={"x": 500, "y": 500}),
                box(
                    "right",
                    10,
                    10,
                    position={"x": 500, "y": 500},
                    align="right",
                ),
            ],
        )

        children = self.resolve(structure)

        self.assertEqual(
            [child["_rel"] for child in children],
            [[5, 5, 10, 10], [25, 25, 10, 10], [45, 45, 10, 10]],
        )
        self.assertEqual([child["_abs"][1] for child in children], [45, 65, 85])

    def test_offset_translates_only_target_without_reflowing_siblings(self) -> None:
        baseline = self.structure(
            size=(100, 40),
            position=(30, 40),
            layout={
                "type": "row",
                "spacing": 10,
                "padding": 5,
                "align": "start",
                "vAlign": "middle",
            },
            children=[box("first", 10, 10), box("target", 10, 10), box("last", 10, 10)],
        )
        nudged = copy.deepcopy(baseline)
        nudged["root"]["children"][1]["offset"] = {"x": 7, "y": -4}

        baseline_children = self.resolve(baseline)
        nudged_children = self.resolve(nudged)

        for index in (0, 2):
            self.assertEqual(nudged_children[index]["_rel"], baseline_children[index]["_rel"])
            self.assertEqual(nudged_children[index]["_abs"], baseline_children[index]["_abs"])
        for field in ("_rel", "_abs"):
            before = baseline_children[1][field]
            after = nudged_children[1][field]
            self.assertEqual(after, [before[0] + 7, before[1] - 4, before[2], before[3]])

    def test_repeated_resolution_is_idempotent_for_nested_layouts(self) -> None:
        structure = {
            "canvas": {"width": 320, "height": 180},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 100, "y": 40},
                "size": {"width": 200, "height": 100},
                "children": [
                    {
                        "type": "container",
                        "name": "panel",
                        "position": {"x": 12, "y": 9},
                        "offset": {"x": 3, "y": -2},
                        "size": {"width": 70, "height": 50},
                        "layout": {
                            "type": "row",
                            "spacing": 5,
                            "padding": 5,
                            "align": "center",
                            "vAlign": "middle",
                        },
                        "children": [
                            box("badge", 10, 10, offset={"x": 2, "y": -1}),
                            box("label", 20, 20),
                        ],
                    }
                ],
            },
        }
        authored = copy.deepcopy(structure)

        layout_resolver.resolve_positions(structure)
        panel = structure["root"]["children"][0]
        self.assertEqual(panel["_rel"], [15, 7, 70, 50])
        self.assertEqual(panel["_abs"], [115, 47, 70, 50])
        self.assert_bbox_relationships(structure["root"])
        expected = self.bbox_snapshot(structure["root"])

        for _ in range(5):
            self.assertIs(layout_resolver.resolve_positions(structure), structure)
            self.assertEqual(self.bbox_snapshot(structure["root"]), expected)
            self.assertEqual(without_resolved_fields(structure), authored)

    def bbox_snapshot(self, root: dict) -> dict[str, tuple[tuple[int, ...], tuple[int, ...]]]:
        snapshot = {}

        def visit(element: dict, path: str) -> None:
            snapshot[path] = (tuple(element["_rel"]), tuple(element["_abs"]))
            for child in element.get("children", []):
                visit(child, f"{path}/{child['name']}")

        visit(root, root["name"])
        return snapshot

    def test_overflow_preserves_sizes_order_and_gap_from_start_edge(self) -> None:
        structure = self.structure(
            size=(50, 40),
            position=(9, 14),
            layout={
                "type": "row",
                "spacing": 8,
                "padding": 5,
                "align": "center",
                "vAlign": "middle",
            },
            children=[box("first", 25, 10), box("second", 25, 10)],
        )

        first, second = self.resolve(structure)

        self.assertEqual(first["_rel"], [5, 15, 25, 10])
        self.assertEqual(first["_abs"], [14, 29, 25, 10])
        self.assertEqual(second["_rel"], [38, 15, 25, 10])
        self.assertEqual(second["_abs"], [47, 29, 25, 10])
        leading, interior, trailing = self.main_axis_gaps(
            structure["root"], axis="x", padding=5
        )
        required_width = sum(child["_rel"][2] for child in (first, second)) + 8
        inner_width = 50 - 2 * 5
        self.assertEqual(leading, 0)
        self.assertEqual(interior, 8)
        self.assertEqual(-trailing, required_width - inner_width)
        self.assertLess(first["_rel"][0], second["_rel"][0])


if __name__ == "__main__":
    unittest.main()
