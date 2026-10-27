from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


ANNOTATE_ELEMENT = (
    Path(__file__).resolve().parents[1] / "scripts" / "annotate_element.py"
)


class AnnotateElementIntegrationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.design = self.root / "design.png"
        self.structure = self.root / "ui_structure.json"
        Image.new("RGB", (220, 180), "black").save(self.design)
        self.structure.write_text(
            json.dumps({
                "canvas": {"width": 220, "height": 180},
                "root": {
                    "type": "container",
                    "name": "root",
                    "position": {"x": 0, "y": 0},
                    "size": {"width": 220, "height": 180},
                    "children": [
                        {
                            "type": "container",
                            "name": "panel",
                            "role": "list",
                            "position": {"x": 20, "y": 25},
                            "size": {"width": 170, "height": 130},
                            "layout": {
                                "type": "row",
                                "padding": {"x": 20, "y": 20},
                                "spacing": 20,
                                "vAlign": "center",
                            },
                            "children": [
                                {
                                    "type": "container",
                                    "name": "first",
                                    "role": "listItem",
                                    "size": {"width": 30, "height": 40},
                                    "children": [{
                                        "type": "rect",
                                        "name": "grandchild",
                                        "position": {"x": 8, "y": 8},
                                        "size": {"width": 8, "height": 8},
                                        "color": "#FFFFFF",
                                    }],
                                },
                                {
                                    "type": "rect",
                                    "name": "second",
                                    "role": "listItem",
                                    "size": {"width": 40, "height": 30},
                                    "color": "#FFFFFF",
                                },
                            ],
                        },
                        {
                            "type": "rect",
                            "name": "standalone",
                            "position": {"x": 195, "y": 30},
                            "size": {"width": 15, "height": 25},
                            "color": "#FFFFFF",
                        },
                    ],
                },
            }),
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def annotate(self, element_path: str) -> tuple[Path, dict]:
        output = self.root / f"{element_path.replace('/', '_')}.png"
        legend = self.root / f"{element_path.replace('/', '_')}_legend.json"
        result = subprocess.run(
            [
                sys.executable,
                "-B",
                str(ANNOTATE_ELEMENT),
                "--design", str(self.design),
                "--structure", str(self.structure),
                "--element-path", element_path,
                "--output", str(output),
                "--legend", str(legend),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode:
            self.fail(f"annotate_element failed:\n{result.stdout}\n{result.stderr}")
        return output, json.loads(legend.read_text(encoding="utf-8"))

    def annotate_all(self) -> dict:
        output = self.root / "all_elements.png"
        legend = self.root / "all_elements_legend.json"
        result = subprocess.run(
            [
                sys.executable,
                "-B",
                str(ANNOTATE_ELEMENT),
                "--design", str(self.design),
                "--structure", str(self.structure),
                "--all-elements",
                "--output", str(output),
                "--legend", str(legend),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode:
            self.fail(f"annotate_element failed:\n{result.stdout}\n{result.stderr}")
        return json.loads(legend.read_text(encoding="utf-8"))

    def test_layout_target_draws_parent_and_direct_children_matching_legend(
        self,
    ) -> None:
        output, legend = self.annotate("root/panel")

        self.assertEqual(legend["element_path"], "root/panel")
        self.assertEqual(legend["target_count"], 3)
        self.assertTrue(legend["is_group"])
        self.assertEqual(legend["targets"], [
            {
                "label": "panel",
                "path": "root/panel",
                "name": "panel",
                "role": "list",
                "abs_bbox": [20, 25, 170, 130],
            },
            {
                "label": "first",
                "path": "root/panel/first",
                "name": "first",
                "role": "listItem",
                "abs_bbox": [40, 70, 30, 40],
            },
            {
                "label": "second",
                "path": "root/panel/second",
                "name": "second",
                "role": "listItem",
                "abs_bbox": [90, 75, 40, 30],
            },
        ])

        with Image.open(output) as annotated:
            pixels = annotated.convert("RGB")
            self.assertEqual(pixels.getpixel((20, 125)), (255, 64, 64))
            self.assertEqual(pixels.getpixel((40, 100)), (64, 200, 80))
            self.assertEqual(pixels.getpixel((90, 95)), (60, 130, 255))
            self.assertEqual(pixels.getpixel((48, 82)), (0, 0, 0))

    def test_all_elements_legend_includes_list_roles(self) -> None:
        legend = self.annotate_all()
        by_path = {item["path"]: item for item in legend["targets"]}

        self.assertEqual(by_path["root/panel"]["role"], "list")
        self.assertEqual(by_path["root/panel/first"]["role"], "listItem")
        self.assertEqual(by_path["root/panel/second"]["role"], "listItem")
        self.assertNotIn("role", by_path["root/standalone"])

    def test_evenly_spaced_named_layout_does_not_gain_list_roles(self) -> None:
        structure = json.loads(self.structure.read_text(encoding="utf-8"))
        panel = structure["root"]["children"][0]
        panel["name"] = "quest_list"
        panel.pop("role")
        panel["layout"]["spacing"] = "even"
        panel["layout"]["align"] = "space-evenly"
        for child in panel["children"]:
            child.pop("role")
        self.structure.write_text(json.dumps(structure), encoding="utf-8")

        legend = self.annotate_all()
        by_path = {item["path"]: item for item in legend["targets"]}

        self.assertNotIn("role", by_path["root/quest_list"])
        self.assertNotIn("role", by_path["root/quest_list/first"])
        self.assertNotIn("role", by_path["root/quest_list/second"])

    def test_non_layout_target_draws_only_itself(self) -> None:
        output, legend = self.annotate("root/standalone")

        self.assertEqual(legend["target_count"], 1)
        self.assertFalse(legend["is_group"])
        self.assertEqual(legend["targets"], [{
            "label": "standalone",
            "path": "root/standalone",
            "name": "standalone",
            "abs_bbox": [195, 30, 15, 25],
        }])
        with Image.open(output) as annotated:
            self.assertEqual(
                annotated.convert("RGB").getpixel((195, 50)),
                (255, 40, 40),
            )


if __name__ == "__main__":
    unittest.main()
