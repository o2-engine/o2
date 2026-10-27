from __future__ import annotations

import sys
import unittest
from pathlib import Path

from PIL import Image, ImageDraw


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

from inventory_assets import alpha_geometry  # noqa: E402
from measure_region import measure_region  # noqa: E402


class RegionMeasurementTests(unittest.TestCase):
    def test_zoom_preserves_pixels_and_maps_back_to_native_coordinates(self) -> None:
        design = Image.new("RGBA", (80, 60))
        design.putdata([
            (x * 3, y * 4, (x + y) % 256, (x * y) % 256)
            for y in range(60) for x in range(80)
        ])
        original = design.tobytes()
        clean, grid, metrics = measure_region(design, (13, 17, 21, 19))
        self.assertEqual(clean.size, (63, 57))
        for cy in range(clean.height):
            for cx in range(clean.width):
                self.assertEqual(
                    clean.getpixel((cx, cy)),
                    design.getpixel((13 + cx // 3, 17 + cy // 3)),
                )
        self.assertEqual(design.tobytes(), original)
        origin = metrics["images"]["design_grid.png"]["content_origin_px"]
        self.assertEqual(grid.size, (63 + origin[0], 57 + origin[1]))
        self.assertEqual(metrics["ticks"]["x"], [
            {"design_px": 20, "crop_px": 21},
            {"design_px": 30, "crop_px": 51},
        ])
        self.assertEqual(metrics["ticks"]["y"], [
            {"design_px": 20, "crop_px": 9},
            {"design_px": 30, "crop_px": 39},
        ])
        # The crop offset is essential; the pixel ruler must not restart at zero.
        for tick in metrics["ticks"]["x"]:
            self.assertEqual(13 + tick["crop_px"] / 3, tick["design_px"])

    def test_region_at_canvas_edge_and_one_pixel_crop_are_supported(self) -> None:
        design = Image.new("RGB", (10, 10), (12, 34, 56))
        clean, _, metrics = measure_region(design, (9, 9, 1, 1), zoom=2)
        self.assertEqual(clean.size, (2, 2))
        self.assertEqual(clean.getpixel((1, 1)), (12, 34, 56, 255))
        self.assertEqual(metrics["ticks"], {"x": [], "y": []})

    def test_invalid_regions_are_rejected_instead_of_silently_padded(self) -> None:
        design = Image.new("RGB", (20, 20))
        for region in ((-1, 0, 5, 5), (0, -1, 5, 5), (0, 0, 0, 5),
                       (0, 0, 5, -1), (18, 0, 5, 5), (0, 18, 5, 5)):
            with self.subTest(region=region), self.assertRaises(ValueError):
                measure_region(design, region)

    def test_invalid_zoom_and_grid_spacing_are_rejected(self) -> None:
        design = Image.new("RGB", (20, 20))
        for options in ({"zoom": 0}, {"zoom": 9}, {"cell_size": 0}, {"cell_size": -1}):
            with self.subTest(options=options), self.assertRaises(ValueError):
                measure_region(design, (0, 0, 10, 10), **options)


class AssetGeometryTests(unittest.TestCase):
    def test_asymmetric_padding_is_separate_from_faint_effect_bounds(self) -> None:
        sprite = Image.new("RGBA", (100, 80))
        draw = ImageDraw.Draw(sprite)
        draw.rectangle((10, 5, 89, 64), fill=(255, 255, 255, 12))
        draw.rectangle((20, 15, 74, 54), fill=(255, 255, 255, 128))
        geometry = alpha_geometry(sprite)
        self.assertEqual(geometry["alpha_bbox"], [10, 5, 90, 65])
        self.assertEqual(geometry["alpha_size"], [80, 60])
        self.assertEqual(geometry["alpha_padding"],
                         {"left": 10, "top": 5, "right": 10, "bottom": 15})
        self.assertEqual(geometry["alpha_core_bbox"], [20, 15, 75, 55])
        self.assertFalse(geometry["fully_transparent"])

    def test_faint_sprite_is_not_confused_with_an_empty_asset(self) -> None:
        faint = alpha_geometry(Image.new("RGBA", (10, 8), (255, 0, 0, 127)))
        empty = alpha_geometry(Image.new("RGBA", (10, 8), (255, 0, 0, 0)))
        self.assertIsNone(faint["alpha_core_bbox"])
        self.assertFalse(faint["fully_transparent"])
        self.assertEqual(faint["alpha_size"], [10, 8])
        self.assertIsNone(empty["alpha_core_bbox"])
        self.assertTrue(empty["fully_transparent"])
        self.assertIsNone(empty["alpha_padding"])
        self.assertEqual(empty["alpha_size"], [0, 0])

    def test_rgb_assets_have_full_image_bounds(self) -> None:
        geometry = alpha_geometry(Image.new("RGB", (13, 7)))
        self.assertEqual(geometry["alpha_bbox"], [0, 0, 13, 7])
        self.assertEqual(geometry["alpha_core_bbox"], [0, 0, 13, 7])
        self.assertEqual(geometry["alpha_padding"],
                         {"left": 0, "top": 0, "right": 0, "bottom": 0})


if __name__ == "__main__":
    unittest.main()
