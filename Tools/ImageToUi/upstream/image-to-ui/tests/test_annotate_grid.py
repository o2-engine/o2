from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
ANNOTATE_GRID = SCRIPTS / "annotate_grid.py"
sys.path.insert(0, str(SCRIPTS))

from annotate_grid import draw_grid, select_grid_palette  # noqa: E402


class AdaptiveGridPaletteTests(unittest.TestCase):
    def test_red_tone_selects_a_bright_cyan_complement(self) -> None:
        palette = select_grid_palette(
            Image.new("RGB", (120, 80), (200, 30, 30))
        )

        r, g, b = palette["line_rgb"]
        self.assertGreater(g, r + 80)
        self.assertGreater(b, r + 80)
        self.assertEqual(palette["halo_rgb"], (0, 0, 0))
        self.assertEqual(palette["contrast_mode"], "bright-on-dark")

    def test_cyan_tone_selects_a_bright_red_complement(self) -> None:
        palette = select_grid_palette(
            Image.new("RGB", (120, 80), (20, 200, 200))
        )

        r, g, b = palette["line_rgb"]
        self.assertGreater(r, g + 80)
        self.assertGreater(r, b + 80)

    def test_light_neutral_art_uses_a_dark_line_with_white_halo(self) -> None:
        palette = select_grid_palette(
            Image.new("RGB", (120, 80), (245, 245, 245))
        )

        self.assertEqual(palette["halo_rgb"], (255, 255, 255))
        self.assertEqual(palette["contrast_mode"], "dark-on-light")
        self.assertLess(max(palette["line_rgb"]), 140)
        self.assertIsNone(palette["dominant_hue_degrees"])

    def test_grid_line_has_a_contrast_halo(self) -> None:
        base_color = (100, 20, 20)
        output = draw_grid(
            Image.new("RGB", (100, 100), base_color),
            target_cell=20,
        ).convert("RGB")

        self.assertNotEqual(output.getpixel((20, 50)), base_color)
        halo = output.getpixel((19, 50))
        self.assertLess(sum(halo), sum(base_color))

    def test_cli_records_the_selected_palette_in_grid_metrics(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            design = root / "design.png"
            output = root / "grid.png"
            metrics_path = root / "metrics.json"
            Image.new("RGB", (120, 80), (200, 30, 30)).save(design)

            result = subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(ANNOTATE_GRID),
                    "--design", str(design),
                    "--output", str(output),
                    "--metrics", str(metrics_path),
                ],
                text=True,
                capture_output=True,
                check=False,
            )

            if result.returncode:
                self.fail(
                    f"annotate_grid failed:\n{result.stdout}\n{result.stderr}"
                )
            style = json.loads(
                metrics_path.read_text(encoding="utf-8")
            )["grid_style"]
            expected = select_grid_palette(
                Image.new("RGB", (120, 80), (200, 30, 30))
            )
            self.assertEqual(style["strategy"], "adaptive-complementary-hue")
            self.assertEqual(style["line_color"], expected["line_hex"])
            self.assertEqual(style["halo_color"], expected["halo_hex"])


if __name__ == "__main__":
    unittest.main()
