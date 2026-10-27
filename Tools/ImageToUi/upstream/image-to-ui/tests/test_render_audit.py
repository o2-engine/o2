from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import audit_render  # noqa: E402
import render_comparison as renderer  # noqa: E402


class RenderTraceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.assets_dir = self.root / "assets"
        self.assets_dir.mkdir()
        Image.new("RGBA", (20, 10), (255, 255, 255, 255)).save(
            self.assets_dir / "icon_probe.png"
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_trace_collects_asset_and_text_metrics_in_render_pass(self) -> None:
        structure = {
            "canvas": {"width": 100, "height": 80},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 100, "height": 80},
                "children": [
                    {
                        "type": "image",
                        "name": "icon",
                        "position": {"x": 5, "y": 5},
                        "size": {"width": 20, "height": 20},
                        "asset": "icon_probe.png",
                    },
                    {
                        "type": "text",
                        "name": "label",
                        "position": {"x": 5, "y": 35},
                        "size": {"width": 80, "height": 30},
                        "text": "OK",
                        "fontSize": 18,
                        "color": "#FFFFFF",
                    },
                ],
            },
        }
        trace: list[dict] = []
        image = renderer.render_from_structure(
            structure,
            renderer.AssetCache(self.assets_dir),
            (0, 0, 0, 0),
            trace,
        )

        self.assertEqual(image.size, (100, 80))
        self.assertEqual([item["path"] for item in trace], [
            "root", "root/icon", "root/label"
        ])
        self.assertEqual([item["z_order"] for item in trace], [0, 1, 2])
        self.assertEqual(
            [item["fully_opaque"] for item in trace],
            [False, True, False],
        )
        self.assertEqual(trace[1]["asset"]["source_size"], [20, 10])
        self.assertAlmostEqual(trace[1]["asset"]["aspect_scale_error"], 1.0)
        self.assertIsNotNone(trace[2]["text"]["ink_bbox"])
        self.assertIn("resolved", trace[2]["text"]["font"])

    def test_text_metrics_report_visible_ink_overflow(self) -> None:
        canvas = Image.new("RGBA", (100, 50), (0, 0, 0, 0))
        metrics = renderer.draw_text(canvas, {
            "type": "text",
            "name": "label",
            "position": {"x": 10, "y": 10},
            "size": {"width": 8, "height": 12},
            "text": "TOO WIDE",
            "fontSize": 20,
            "color": "#FFFFFF",
        }, 0, 0)
        self.assertIsNotNone(metrics)
        self.assertGreater(max(metrics["ink_overflow"].values()), 1)


class AuditTests(unittest.TestCase):
    def test_audit_separates_blocking_errors_from_diagnostic_warnings(self) -> None:
        trace = {
            "elements": [
                {
                    "path": "root/label",
                    "bbox": [0, 0, 20, 10],
                    "visible_bbox": [0, 0, 20, 10],
                    "parent_bbox": [0, 0, 100, 100],
                    "type": "text",
                    "text": {
                        "font": {
                            "requested": "Missing.ttf",
                            "resolved": "C:/Windows/Fonts/arial.ttf",
                            "fallback": True,
                        },
                        "ink_overflow": {"left": 0, "top": 0, "right": 3, "bottom": 0},
                        "ink_bbox": [0, 0, 23, 10],
                    },
                },
                {
                    "path": "root/icon",
                    "bbox": [90, 20, 20, 20],
                    "visible_bbox": [90, 20, 10, 20],
                    "parent_bbox": [0, 0, 100, 100],
                    "type": "image",
                    "asset": {
                        "requested": "icon_probe.png",
                        "resolved": "icon_probe.png",
                        "found": True,
                        "render_mode": "stretch",
                        "aspect_scale_error": 0.2,
                    },
                },
            ]
        }
        inventory = {
            "assets": [{
                "path": "icon_probe.png",
                "basename": "icon_probe.png",
                "likely_usage": "icon",
            }]
        }

        result = audit_render.audit_trace(trace, inventory)

        self.assertFalse(result["valid"])
        self.assertEqual(result["error_count"], 2)
        self.assertEqual(result["warning_count"], 2)
        self.assertEqual(
            {item["code"] for item in result["issues"]},
            {"font_fallback", "text_ink_overflow", "atomic_asset_aspect", "parent_overflow"},
        )

    def test_audit_blocks_trace_with_only_logical_nodes(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root",
                    "bbox": [0, 0, 100, 80],
                    "type": "container",
                },
                {
                    "path": "root/popup",
                    "bbox": [10, 10, 80, 60],
                    "parent_bbox": [0, 0, 100, 80],
                    "type": "container",
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertFalse(result["valid"])
        self.assertEqual(result["error_count"], 1)
        self.assertEqual(result["issues"][0]["code"], "no_visible_foreground")

    def test_audit_blocks_a_lone_full_canvas_scrim(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root/modal_scrim",
                    "name": "modal_scrim",
                    "bbox": [0, 0, 100, 80],
                    "visible_bbox": [0, 0, 100, 80],
                    "type": "rect",
                    "opacity": 0.6,
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertFalse(result["valid"])
        self.assertEqual(result["error_count"], 1)
        self.assertEqual(result["issues"][0]["code"], "no_visible_foreground")

    def test_audit_accepts_visible_foreground_above_a_full_canvas_scrim(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root/modal_scrim",
                    "name": "modal_scrim",
                    "bbox": [0, 0, 100, 80],
                    "visible_bbox": [0, 0, 100, 80],
                    "type": "rect",
                    "opacity": 0.6,
                },
                {
                    "path": "root/popup/panel",
                    "name": "panel",
                    "bbox": [20, 15, 60, 50],
                    "visible_bbox": [20, 15, 60, 50],
                    "type": "image",
                    "opacity": 1.0,
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertTrue(result["valid"])
        self.assertEqual(result["issues"], [])

    def test_translucent_full_canvas_image_is_not_implicitly_a_scrim(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root/tutorial_art",
                    "name": "tutorial_art",
                    "bbox": [0, 0, 100, 80],
                    "visible_bbox": [0, 0, 100, 80],
                    "type": "image",
                    "opacity": 0.8,
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertTrue(result["valid"])
        self.assertEqual(result["issues"], [])

    def test_later_fully_opaque_scrim_excludes_hidden_earlier_foreground(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root/final_scrim",
                    "name": "final_scrim",
                    "bbox": [0, 0, 100, 80],
                    "visible_bbox": [0, 0, 100, 80],
                    "type": "overlay",
                    "opacity": 1.0,
                    "z_order": 2,
                    "fully_opaque": True,
                },
                {
                    "path": "root/hidden_panel",
                    "name": "hidden_panel",
                    "bbox": [20, 15, 60, 50],
                    "visible_bbox": [20, 15, 60, 50],
                    "type": "image",
                    "opacity": 1.0,
                    "z_order": 1,
                    "fully_opaque": True,
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertFalse(result["valid"])
        foreground_issue = next(
            item for item in result["issues"]
            if item["code"] == "no_visible_foreground"
        )
        self.assertEqual(
            foreground_issue["details"]["excluded_fully_occluded"],
            ["root/hidden_panel"],
        )

    def test_transparent_hole_layer_is_not_treated_as_opaque_coverage(self) -> None:
        trace = {
            "canvas": {"width": 100, "height": 80},
            "elements": [
                {
                    "path": "root/panel",
                    "name": "panel",
                    "bbox": [20, 15, 60, 50],
                    "visible_bbox": [20, 15, 60, 50],
                    "type": "image",
                    "opacity": 1.0,
                    "z_order": 1,
                    "fully_opaque": True,
                },
                {
                    "path": "root/holey_overlay",
                    "name": "holey_overlay",
                    "bbox": [0, 0, 100, 80],
                    "visible_bbox": [0, 0, 100, 80],
                    "type": "overlay",
                    "opacity": 1.0,
                    "z_order": 2,
                    "fully_opaque": False,
                },
            ],
        }

        result = audit_render.audit_trace(trace, {"assets": []})

        self.assertTrue(result["valid"])
        self.assertEqual(result["issues"], [])


if __name__ == "__main__":
    unittest.main()
