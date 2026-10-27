from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import render_comparison as renderer  # noqa: E402
import validate_structure as structure_validator  # noqa: E402


class TextRenderingTests(unittest.TestCase):
    def setUp(self) -> None:
        self.font_size = 42
        self.font = renderer.find_font(self.font_size)
        self.original_find_font = renderer.find_font
        renderer.find_font = lambda _size, _preferred=None: self.font

    def tearDown(self) -> None:
        renderer.find_font = self.original_find_font

    def draw_case(
        self,
        *,
        text: str = "PLAY",
        text_scale_x: float | None = None,
        alignment: str = "center",
        text_v_align: str = "middle",
        line_height: int | None = None,
        canvas_size: tuple[int, int] = (240, 200),
        position: tuple[int, int] = (20, 20),
        size: tuple[int, int] = (200, 160),
    ) -> tuple[Image.Image, dict]:
        canvas = Image.new("RGBA", canvas_size, (0, 0, 0, 0))
        element = {
            "type": "text",
            "name": "probe",
            "position": {"x": position[0], "y": position[1]},
            "size": {"width": size[0], "height": size[1]},
            "text": text,
            "fontSize": self.font_size,
            "color": "#FFFFFF",
            "strokeColor": "#000000",
            "strokeWidth": 3,
            "alignment": alignment,
            "textVAlign": text_v_align,
        }
        if text_scale_x is not None:
            element["textScaleX"] = text_scale_x
        if line_height is not None:
            element["lineHeight"] = line_height
        metrics = renderer.draw_text(canvas, element, 0, 0)
        self.assertIsNotNone(metrics)
        return canvas, metrics

    def alpha_bbox(self, image: Image.Image) -> list[int] | None:
        bbox = image.getchannel("A").getbbox()
        if bbox is None:
            return None
        left, top, right, bottom = bbox
        return [left, top, right - left, bottom - top]

    def render_bbox(
        self,
        text: str,
        *,
        text_v_align: str,
        alignment: str = "center",
        line_height: int | None = None,
    ) -> tuple[int, int, int, int]:
        canvas = Image.new("RGBA", (240, 160), (0, 0, 0, 0))
        element = {
            "type": "text",
            "name": "probe",
            "position": {"x": 20, "y": 20},
            "size": {"width": 200, "height": 120},
            "text": text,
            "fontSize": self.font_size,
            "color": "#FFFFFF",
            "strokeColor": "#000000",
            "strokeWidth": 3,
            "alignment": alignment,
            "textVAlign": text_v_align,
        }
        if line_height is not None:
            element["lineHeight"] = line_height
        renderer.draw_text(canvas, element, 0, 0)
        bbox = canvas.getchannel("A").getbbox()
        self.assertIsNotNone(bbox)
        return bbox  # type: ignore[return-value]

    def test_top_alignment_uses_visible_ink_top(self) -> None:
        _left, top, _right, _bottom = self.render_bbox(
            "PLAY", text_v_align="top"
        )
        self.assertEqual(top, 20)

    def test_middle_alignment_centers_visible_ink(self) -> None:
        left, top, right, bottom = self.render_bbox(
            "PLAY", text_v_align="middle"
        )
        self.assertLessEqual(abs((left + right) - 240), 2)
        self.assertLessEqual(abs((top + bottom) - 160), 1)

    def test_bottom_alignment_uses_visible_ink_bottom(self) -> None:
        _left, _top, _right, bottom = self.render_bbox(
            "PLAY", text_v_align="bottom"
        )
        self.assertEqual(bottom, 140)

    def test_multiline_middle_alignment_centers_visible_block(self) -> None:
        _left, top, _right, bottom = self.render_bbox(
            "LINE ONE\nLINE TWO", text_v_align="middle", line_height=48
        )
        self.assertLessEqual(abs((top + bottom) - 160), 1)

    def test_right_alignment_ignores_trailing_space_advance(self) -> None:
        plain_canvas, plain_metrics = self.draw_case(
            text="PLAY", alignment="right"
        )
        spaced_canvas, spaced_metrics = self.draw_case(
            text="PLAY   ", alignment="right"
        )

        self.assertEqual(self.alpha_bbox(spaced_canvas), self.alpha_bbox(plain_canvas))
        self.assertEqual(
            spaced_metrics["line_ink_bboxes"], plain_metrics["line_ink_bboxes"]
        )
        self.assertEqual(spaced_metrics["ink_bbox"], plain_metrics["ink_bbox"])

    def test_left_alignment_ignores_leading_space_advance(self) -> None:
        plain_canvas, plain_metrics = self.draw_case(text="PLAY", alignment="left")
        spaced_canvas, spaced_metrics = self.draw_case(
            text="   PLAY", alignment="left"
        )

        self.assertEqual(self.alpha_bbox(spaced_canvas), self.alpha_bbox(plain_canvas))
        self.assertEqual(
            spaced_metrics["line_ink_bboxes"], plain_metrics["line_ink_bboxes"]
        )
        self.assertEqual(spaced_metrics["ink_bbox"], plain_metrics["ink_bbox"])

    def test_multiline_alignment_uses_each_lines_visible_ink(self) -> None:
        for alignment in ("left", "center", "right"):
            for text_v_align in ("top", "middle", "bottom"):
                with self.subTest(
                    alignment=alignment, text_v_align=text_v_align
                ):
                    plain_canvas, plain_metrics = self.draw_case(
                        text="ONE\nTWO",
                        alignment=alignment,
                        text_v_align=text_v_align,
                        line_height=52,
                    )
                    spaced_canvas, spaced_metrics = self.draw_case(
                        text="  ONE \n TWO   ",
                        alignment=alignment,
                        text_v_align=text_v_align,
                        line_height=52,
                    )

                    self.assertEqual(
                        self.alpha_bbox(spaced_canvas), self.alpha_bbox(plain_canvas)
                    )
                    self.assertEqual(
                        spaced_metrics["line_ink_bboxes"],
                        plain_metrics["line_ink_bboxes"],
                    )
                    self.assertEqual(
                        spaced_metrics["visible_bbox"], plain_metrics["visible_bbox"]
                    )

    def test_explicit_unit_text_scale_preserves_default_pixels(self) -> None:
        default_canvas, default_metrics = self.draw_case(text="PIXEL EXACT")
        unit_canvas, unit_metrics = self.draw_case(
            text="PIXEL EXACT", text_scale_x=1.0
        )

        self.assertEqual(default_canvas.tobytes(), unit_canvas.tobytes())
        self.assertEqual(default_metrics, unit_metrics)
        self.assertEqual(default_metrics["text_scale_x"], 1.0)

    def test_text_scale_x_scales_visible_width_about_alignment(self) -> None:
        base_canvas, base_metrics = self.draw_case(text="WIDTH")
        base_line = base_metrics["line_ink_bboxes"][0]
        self.assertIsNotNone(base_line)
        self.assertEqual(base_metrics["visible_bbox"], self.alpha_bbox(base_canvas))

        for scale in (0.5, 1.5):
            with self.subTest(scale=scale):
                canvas, metrics = self.draw_case(text="WIDTH", text_scale_x=scale)
                line = metrics["line_ink_bboxes"][0]
                self.assertIsNotNone(line)
                self.assertEqual(line[2], max(1, round(base_line[2] * scale)))
                self.assertEqual(line[3], base_line[3])
                self.assertLessEqual(abs((line[0] * 2 + line[2]) - 240), 1)
                self.assertEqual(metrics["text_scale_x"], scale)
                self.assertEqual(metrics["visible_bbox"], self.alpha_bbox(canvas))

    def test_multiline_trace_records_tight_bbox_for_every_line(self) -> None:
        canvas, metrics = self.draw_case(
            text="WIDE\n\nI",
            alignment="left",
            text_v_align="top",
            line_height=60,
            canvas_size=(260, 240),
            size=(220, 200),
        )

        line_boxes = metrics["line_ink_bboxes"]
        self.assertEqual(len(line_boxes), 3)
        self.assertIsNone(line_boxes[1])
        visible_lines = [box for box in line_boxes if box is not None]
        for left, top, width, height in visible_lines:
            self.assertEqual(
                canvas.getchannel("A").crop(
                    (left, top, left + width, top + height)
                ).getbbox(),
                (0, 0, width, height),
            )

        left = min(box[0] for box in visible_lines)
        top = min(box[1] for box in visible_lines)
        right = max(box[0] + box[2] for box in visible_lines)
        bottom = max(box[1] + box[3] for box in visible_lines)
        expected_union = [left, top, right - left, bottom - top]
        self.assertEqual(metrics["ink_bbox"], expected_union)
        self.assertEqual(metrics["visible_bbox"], self.alpha_bbox(canvas))

    def test_trace_records_visible_bbox_for_image_rect_and_text(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            assets_dir = Path(temporary)
            sprite = Image.new("RGBA", (4, 4), (0, 0, 0, 0))
            sprite.paste((255, 0, 0, 255), (1, 1, 3, 4))
            sprite.save(assets_dir / "probe.png")
            assets = renderer.AssetCache(assets_dir)

            def render_root(root: dict, canvas_size: tuple[int, int]):
                trace: list[dict] = []
                image = renderer.render_from_structure(
                    {
                        "canvas": {
                            "width": canvas_size[0],
                            "height": canvas_size[1],
                        },
                        "root": root,
                    },
                    assets,
                    (0, 0, 0, 0),
                    trace,
                )
                return image, trace[0]

            image, image_trace = render_root({
                "type": "image",
                "name": "image",
                "position": {"x": 2, "y": 1},
                "size": {"width": 4, "height": 4},
                "asset": "probe.png",
            }, (8, 8))
            self.assertEqual(image_trace["visible_bbox"], [3, 2, 2, 3])
            self.assertEqual(image_trace["visible_bbox"], self.alpha_bbox(image))

            rect, rect_trace = render_root({
                "type": "rect",
                "name": "rect",
                "position": {"x": 5, "y": 6},
                "size": {"width": 6, "height": 4},
                "color": "#00FF00",
            }, (8, 8))
            self.assertEqual(rect_trace["visible_bbox"], [5, 6, 3, 2])
            self.assertEqual(rect_trace["visible_bbox"], self.alpha_bbox(rect))

            for element_type in ("container", "button", "overlay"):
                with self.subTest(element_type=element_type, visual="asset"):
                    asset_image, asset_trace = render_root({
                        "type": element_type,
                        "name": element_type,
                        "position": {"x": 2, "y": 1},
                        "size": {"width": 4, "height": 4},
                        "asset": "probe.png",
                    }, (8, 8))
                    self.assertEqual(asset_trace["visible_bbox"], [3, 2, 2, 3])
                    self.assertEqual(
                        asset_trace["visible_bbox"], self.alpha_bbox(asset_image)
                    )
                    self.assertFalse(asset_trace["fully_opaque"])

                with self.subTest(element_type=element_type, visual="color"):
                    color_image, color_trace = render_root({
                        "type": element_type,
                        "name": element_type,
                        "position": {"x": 5, "y": 6},
                        "size": {"width": 6, "height": 4},
                        "color": "#00FF00",
                    }, (8, 8))
                    self.assertEqual(color_trace["visible_bbox"], [5, 6, 3, 2])
                    self.assertEqual(
                        color_trace["visible_bbox"], self.alpha_bbox(color_image)
                    )
                    self.assertTrue(color_trace["fully_opaque"])

            text, text_trace = render_root({
                "type": "text",
                "name": "text",
                "position": {"x": 20, "y": 20},
                "size": {"width": 200, "height": 120},
                "text": "TRACE",
                "fontSize": self.font_size,
                "color": "#FFFFFF",
                "alignment": "center",
                "textVAlign": "middle",
            }, (240, 160))
            self.assertEqual(text_trace["visible_bbox"], self.alpha_bbox(text))
            self.assertEqual(
                text_trace["text"]["visible_bbox"],
                text_trace["visible_bbox"],
            )


class TextValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.design = self.root / "design.png"
        self.assets = self.root / "assets"
        self.assets.mkdir()
        Image.new("RGB", (32, 32), "black").save(self.design)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def validate_text(self, **fields: object) -> list[str]:
        text = {
            "type": "text",
            "name": "label",
            "position": {"x": 0, "y": 0},
            "size": {"width": 20, "height": 10},
            "anchor": {"horizontal": "left", "vertical": "top"},
            "text": "OK",
            **fields,
        }
        structure = {
            "canvas": {"width": 32, "height": 32},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 32, "height": 32},
                "anchor": {"horizontal": "left", "vertical": "top"},
                "children": [text],
            },
        }
        reporter, _stats = structure_validator.validate_structure(
            structure, self.design, self.assets
        )
        return reporter.errors

    def test_rejects_non_positive_line_height(self) -> None:
        errors = self.validate_text(lineHeight=0)
        self.assertTrue(any("lineHeight must be positive" in item for item in errors))

    def test_rejects_negative_stroke_width(self) -> None:
        errors = self.validate_text(strokeWidth=-1)
        self.assertTrue(
            any("strokeWidth must be a non-negative number" in item for item in errors)
        )

    def test_accepts_valid_text_metrics(self) -> None:
        self.assertEqual(self.validate_text(lineHeight=48, strokeWidth=3), [])

    def test_rejects_non_positive_or_non_finite_text_scale_x(self) -> None:
        for value in (0, -1, float("inf"), float("-inf"), float("nan"), True, "1"):
            with self.subTest(value=value):
                errors = self.validate_text(textScaleX=value)
                self.assertTrue(
                    any(
                        "textScaleX must be a positive finite number" in item
                        for item in errors
                    ),
                    errors,
                )

    def test_accepts_positive_finite_text_scale_x(self) -> None:
        for value in (0.25, 1, 2.0):
            with self.subTest(value=value):
                self.assertEqual(self.validate_text(textScaleX=value), [])


if __name__ == "__main__":
    unittest.main()
