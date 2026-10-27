from __future__ import annotations

import copy
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import audit_render
import layout
import render_comparison as renderer
import scale_structure
import ui_components as components
import validate_structure


ANCHOR = {"horizontal": "left", "vertical": "top"}


def node(name, box, *, color=None, children=None, **extra):
    x, y, w, h = box
    result = {"type": "container" if children is not None else "rect", "name": name,
              "position": {"x": x, "y": y}, "size": {"width": w, "height": h},
              "anchor": dict(ANCHOR)}
    if color:
        result["color"] = color
    if children is not None:
        result["children"] = children
    result.update(extra)
    return result


def scene(children, width=100, height=80):
    return {"canvas": {"width": width, "height": height},
            "root": node("root", (0, 0, width, height), children=children)}


def constraints(low, high, first, last, **extra):
    pair = lambda xy: {"x": xy[0], "y": xy[1]}
    return {"min": pair(low), "max": pair(high), "offsetMin": pair(first),
            "offsetMax": pair(last), **extra}


def meter(direction="left-to-right", value=0.5):
    return node("meter", (10, 10, 20, 10), children=[
        node("track", (0, 0, 20, 10), color="#303030"),
        node("fill", (0, 0, 20, 10), color="#FFFFFF"),
    ], progress={"fill": "fill", "value": value, "direction": direction})


class ComponentTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.assets = Path(self.temp.name)
        self.design = self.assets / "design.png"
        Image.new("RGB", (100, 80)).save(self.design)

    def tearDown(self):
        self.temp.cleanup()

    def render(self, structure, **scenario):
        selected = components.snapshot(structure, **scenario)
        trace = []
        image = renderer.render_from_structure(selected, renderer.AssetCache(self.assets), (0, 0, 0, 0), trace)
        return image, {item["path"]: item for item in trace}

    def validate(self, structure):
        return validate_structure.validate_structure(structure, None, self.assets)[0]

    def test_anchor_backfill_prefers_declared_responsive_attachment(self):
        import backfill_anchors
        panel = node("panel", (0, 0, 10, 10), color="#FFFFFF",
                     responsive=constraints((1, 1), (1, 1), (-99, -40), (-5, -10)))
        panel.pop("anchor")
        original = scene([panel])
        filled, _ = backfill_anchors.backfill_anchors(original)
        self.assertEqual(filled["root"]["children"][0]["anchor"],
                         {"horizontal": "right", "vertical": "bottom"})
        self.assertNotIn("_clip_bbox", filled["root"]["children"][0])
        self.assertNotIn("_abs", filled["root"]["children"][0])

    def test_bottom_right_anchor_keeps_fixed_margins_at_multiple_sizes(self):
        panel = node("panel", (70, 55, 20, 15), color="#FFFFFF",
                     responsive=constraints((1, 1), (1, 1), (-30, -25), (-10, -10)))
        original = scene([panel])
        before = copy.deepcopy(original)
        for size in ((100, 80), (160, 120), (90, 70)):
            with self.subTest(size=size):
                _, trace = self.render(original, size=size)
                self.assertEqual(trace["root/panel"]["bbox"], [size[0]-30, size[1]-25, 20, 15])
        self.assertEqual(original, before)

    def test_safe_area_stretch_and_nested_layout_use_resolved_parent_size(self):
        group = node("group", (0, 0, 10, 10), children=[
            node("left", (0, 0, 10, 10), color="#FFFFFF"),
            node("right", (0, 0, 10, 10), color="#FFFFFF"),
        ], responsive=constraints((0, 0), (1, 1), (2, 2), (-2, -2), safeArea=True),
           layout={"type": "row", "align": "space-between"})
        for child in group["children"]:
            child.pop("position")
        original = scene([group])
        original["canvas"]["safeArea"] = {"left": 5, "right": 7, "top": 3, "bottom": 9}
        self.assertEqual(self.validate(original).errors, [])
        _, trace = self.render(original)
        self.assertEqual(trace["root/group"]["bbox"], [7, 5, 84, 64])
        self.assertEqual(trace["root/group/right"]["bbox"], [81, 5, 10, 10])

    def test_responsive_and_layout_conflicts_are_rejected(self):
        original = scene([node("probe", (0, 0, 10, 10), color="#FFFFFF",
                              responsive=constraints((0, 0), (1, 0), (0, 0), (0, 10)))])
        original["root"]["layout"] = {"type": "row"}
        self.assertTrue(any("cannot compete" in error for error in self.validate(original).errors))

    def test_non_positive_responsive_extent_is_rejected(self):
        original = scene([node("probe", (0, 0, 10, 10), color="#FFFFFF",
                              responsive=constraints((0, 0), (1, 1), (60, 0), (-60, 0)))])
        self.assertTrue(any("non-positive" in error for error in self.validate(original).errors))

    def test_switching_variants_restores_baseline_and_preserves_authored_data(self):
        control = node("toggle", (10, 10, 20, 10), children=[
            node("base", (0, 0, 20, 10), color="#FF0000"),
            node("badge", (0, 0, 3, 3), color="#FFFFFF", visible=False),
        ], state={"current": "normal", "variants": {
            "normal": {},
            "selected": {"base": {"color": "#00FF00"}, "badge": {"visible": True}},
            "hidden": {".": {"visible": False}},
        }})
        original = scene([control])
        before = copy.deepcopy(original)
        baseline, _ = self.render(original)
        selected, _ = self.render(original, states={"root/toggle": "selected"})
        hidden, _ = self.render(original, states={"root/toggle": "hidden"})
        restored, _ = self.render(original)
        self.assertEqual(baseline.getpixel((10, 10)), (255, 0, 0, 255))
        self.assertEqual(selected.getpixel((10, 10)), (255, 255, 255, 255))
        self.assertEqual(selected.getpixel((15, 15)), (0, 255, 0, 255))
        self.assertIsNone(hidden.getchannel("A").getbbox())
        self.assertEqual(restored.tobytes(), baseline.tobytes())
        self.assertEqual(original, before)

    def test_inactive_variant_assets_are_validated(self):
        original = scene([node("icon", (0, 0, 10, 10), type="image", asset="design.png",
                              state={"current": "normal", "variants": {
                                  "normal": {}, "selected": {".": {"asset": "missing.png"}}}})])
        self.assertTrue(any("asset not found: missing.png" in error for error in self.validate(original).errors))

    def test_variant_on_layout_child_keeps_original_validation_context(self):
        child = node("slot", (0, 0, 10, 10), color="#FFFFFF",
                     state={"current": "normal", "variants": {
                         "normal": {}, "selected": {".": {"color": "#00FF00"}}}})
        child.pop("position")
        original = scene([child])
        original["root"]["layout"] = {"type": "row"}
        report = self.validate(original)
        self.assertEqual(report.errors, [])
        self.assertEqual(report.warnings, [])

    def test_invalid_component_targets_values_and_overrides_are_rejected(self):
        bad_specs = [
            {"progress": {"fill": "missing", "value": 0.5, "direction": "left-to-right"}},
            {"progress": {"fill": "fill", "value": True, "direction": "left-to-right"}},
            {"progress": {"fill": "fill", "value": float("nan"), "direction": "left-to-right"}},
            {"progress": {"fill": "fill", "value": 2, "direction": "left-to-right"}},
            {"progress": {"fill": "fill", "min": 1, "max": 1, "value": 1, "direction": "left-to-right"}},
            {"state": {"current": "unknown", "variants": {"normal": {}}}},
            {"state": {"current": "normal", "variants": {"normal": {".": {"size": {"width": 1}}}}}},
        ]
        for spec in bad_specs:
            with self.subTest(spec=spec):
                bar = meter()
                bar.update(spec)
                self.assertTrue(self.validate(scene([bar])).errors)

    def test_progress_empty_partial_full_in_all_four_directions(self):
        for direction in sorted(components.DIRECTIONS):
            for value in (0, 0.5, 1):
                with self.subTest(direction=direction, value=value):
                    image, trace = self.render(scene([meter(direction, value)]))
                    white = [(x, y) for y in range(10, 20) for x in range(10, 30)
                             if image.getpixel((x, y)) == (255, 255, 255, 255)]
                    self.assertEqual(len(white), int(200*value))
                    self.assertEqual(trace["root/meter/fill"]["bbox"], [10, 10, 20, 10])
                    if value == 0:
                        self.assertIsNone(trace["root/meter/fill"]["visible_bbox"])
                    if value == 0.5:
                        expected = {
                            "left-to-right": [10, 10, 10, 10], "right-to-left": [20, 10, 10, 10],
                            "top-to-bottom": [10, 10, 20, 5], "bottom-to-top": [10, 15, 20, 5],
                        }[direction]
                        self.assertEqual(trace["root/meter/fill"]["visible_bbox"], expected)

    def test_progress_clips_texture_without_squeezing_its_pixels(self):
        sprite = Image.new("RGBA", (20, 10))
        sprite.putdata([(x*10, y*20, 0, 255) for y in range(10) for x in range(20)])
        sprite.save(self.assets / "gradient.png")
        bar = meter()
        fill = bar["children"][1]
        fill.pop("color")
        fill.update(type="image", asset="gradient.png")
        image, _ = self.render(scene([bar]))
        self.assertEqual(image.getpixel((19, 14)), sprite.getpixel((9, 4)))
        self.assertEqual(image.getpixel((20, 14)), (48, 48, 48, 255))

    def test_progress_label_uses_declared_numeric_range(self):
        bar = meter()
        bar["children"].append(node("label", (0, 15, 80, 20), type="text", text="50%",
                                    color="#FFFFFF", fontSize=12))
        bar["progress"].update(min=20, max=120, value=70, label="label", format="{value:.0f}/{max:.0f} ({percent:.0f}%)")
        resolved = components.snapshot(scene([bar]))
        self.assertEqual(resolved["root"]["children"][0]["children"][2]["text"], "70/120 (50%)")
        bar["progress"]["format"] = "{value.real}"
        self.assertTrue(self.validate(scene([bar])).errors)

    def test_scroll_offsets_clip_pixels_and_do_not_produce_false_overflow(self):
        content = node("content", (0, 0, 20, 40), children=[
            node("red", (0, 0, 20, 20), color="#FF0000"),
            node("green", (0, 20, 20, 20), color="#00FF00"),
        ])
        viewport = node("viewport", (10, 10, 20, 20), children=[content],
                        scroll={"content": "content", "direction": "vertical", "offset": {"x": 0, "y": 10}})
        image, trace = self.render(scene([viewport]))
        self.assertEqual(image.getpixel((15, 10)), (255, 0, 0, 255))
        self.assertEqual(image.getpixel((15, 25)), (0, 255, 0, 255))
        self.assertEqual(image.getpixel((15, 30))[3], 0)
        self.assertEqual(trace["root/viewport/content/red"]["visible_bbox"], [10, 10, 20, 10])
        report = audit_render.audit_trace({"canvas": {"width": 100, "height": 80}, "elements": list(trace.values())}, {"assets": []})
        self.assertEqual(report["issues"], [])

    def test_nested_progress_and_scroll_clips_intersect(self):
        bar = meter()
        bar["position"] = {"x": 0, "y": 5}
        content = node("content", (0, 0, 20, 40), children=[bar])
        viewport = node("viewport", (10, 10, 20, 20), children=[content],
                        scroll={"content": "content", "direction": "vertical", "offset": {"x": 0, "y": 10}})
        image, trace = self.render(scene([viewport]))
        self.assertEqual(trace["root/viewport/content/meter/fill"]["visible_bbox"], [10, 10, 10, 5])
        self.assertEqual(image.getpixel((15, 9))[3], 0)
        self.assertEqual(image.getpixel((15, 10)), (255, 255, 255, 255))

    def test_clip_trace_uses_actual_alpha_when_only_transparent_holes_remain(self):
        sprite = Image.new("RGBA", (8, 8))
        sprite.putpixel((0, 0), (255, 255, 255, 255))
        sprite.putpixel((7, 7), (255, 255, 255, 255))
        sprite.save(self.assets / "sparse.png")
        content = node("content", (0, 0, 8, 8), children=[
            node("sparse", (0, 0, 8, 8), type="image", asset="sparse.png"),
        ])
        viewport = node("viewport", (10, 10, 8, 2), children=[content],
                        scroll={"content": "content", "direction": "vertical", "offset": {"x": 0, "y": 3}})
        image, trace = self.render(scene([viewport]))
        self.assertIsNone(image.getchannel("A").getbbox())
        self.assertIsNone(trace["root/viewport/content/sparse"]["visible_bbox"])

    def test_scroll_offset_clamps_when_viewport_grows(self):
        content = node("content", (0, 0, 20, 40), color="#FFFFFF", children=[])
        viewport = node("viewport", (0, 0, 20, 20), children=[content],
                        responsive=constraints((0, 0), (0, 1), (0, 0), (20, 0)),
                        scroll={"content": "content", "direction": "vertical", "offset": {"x": 0, "y": 100}})
        _, trace = self.render(scene([viewport]), size=(100, 30))
        self.assertEqual(trace["root/viewport"]["scroll"]["offset"], {"x": 0, "y": 10})
        _, trace = self.render(scene([viewport]), size=(100, 60))
        self.assertEqual(trace["root/viewport"]["scroll"]["offset"], {"x": 0, "y": 0})

    def test_scaling_keeps_ratios_values_and_scales_only_pixel_constraints(self):
        bar = meter()
        bar["responsive"] = constraints((0, 0), (1, 0), (10, 5), (-10, 15))
        original = scene([bar])
        original["canvas"]["safeArea"] = {"left": 3, "top": 4}
        scaled = scale_structure.scale_structure(original, 2, 3)
        new_bar = scaled["root"]["children"][0]
        self.assertEqual(new_bar["responsive"]["max"], {"x": 1, "y": 0})
        self.assertEqual(new_bar["responsive"]["offsetMax"], {"x": -20, "y": 45})
        self.assertEqual(new_bar["progress"]["value"], 0.5)
        self.assertEqual(scaled["canvas"]["safeArea"], {"left": 6, "top": 12})


if __name__ == "__main__":
    unittest.main()
