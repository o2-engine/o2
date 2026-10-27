from __future__ import annotations

import hashlib
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import PIL
from PIL import Image, ImageFont


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import render_comparison as renderer  # noqa: E402


REGION_COLORS = (
    ((211, 31, 42, 255), (229, 146, 37, 255), (247, 214, 59, 255)),
    ((39, 158, 78, 255), (29, 132, 198, 255), (60, 72, 181, 255)),
    ((132, 61, 174, 255), (220, 72, 155, 255), (112, 73, 48, 255)),
)


class FontProbe:
    def __init__(self, path: Path, family: str, style: str) -> None:
        self.path = str(path)
        self._name = (family, style)

    def getname(self) -> tuple[str, str]:
        return self._name


def painted_nine_regions(size: tuple[int, int], margins: dict[str, int]) -> Image.Image:
    """Create a deterministic nine-region image suitable for pixel goldens."""
    width, height = size
    xs = (0, margins["left"], width - margins["right"], width)
    ys = (0, margins["top"], height - margins["bottom"], height)
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    for row in range(3):
        for column in range(3):
            image.paste(
                REGION_COLORS[row][column],
                (xs[column], ys[row], xs[column + 1], ys[row + 1]),
            )
    return image


def coordinate_pattern(size: tuple[int, int]) -> Image.Image:
    """Give every source coordinate a distinct color for margin regressions."""
    width, height = size
    image = Image.new("RGBA", size)
    image.putdata([
        (
            (x * 47 + y * 13) % 256,
            (x * 19 + y * 61) % 256,
            (x * 83 + y * 29) % 256,
            255,
        )
        for y in range(height)
        for x in range(width)
    ])
    return image


class FontTraceStabilityTests(unittest.TestCase):
    @staticmethod
    def _font_loader(path: str, _size: int) -> FontProbe:
        return FontProbe(Path(path).resolve(), "Probe", "Regular")

    def test_explicit_regular_filename_matches_exact_resolved_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            font_path = Path(temporary) / "arial.ttf"
            font_bytes = b"regular-font-probe"
            font_path.write_bytes(font_bytes)

            with mock.patch.object(
                renderer, "FONT_SEARCH_DIRS", [Path(temporary)]
            ):
                details = renderer.describe_font(
                    FontProbe(font_path, "Arial", "Regular"),  # type: ignore[arg-type]
                    "arial.ttf",
                )

        self.assertFalse(details["fallback"])
        self.assertEqual(
            details["resolved_sha256"], hashlib.sha256(font_bytes).hexdigest()
        )
        self.assertEqual(details["pillow_version"], PIL.__version__)

    def test_explicit_regular_filename_rejects_resolved_bold_file(self) -> None:
        bold_path = Path("C:/Windows/Fonts/arialbd.ttf")

        details = renderer.describe_font(
            FontProbe(bold_path, "Arial", "Bold"),  # type: ignore[arg-type]
            "arial.ttf",
        )

        self.assertTrue(details["fallback"])

    def test_memory_backed_default_font_has_stable_non_file_trace(self) -> None:
        details = [
            renderer.describe_font(ImageFont.load_default(), None)
            for _ in range(2)
        ]

        self.assertEqual(details[0], details[1])
        self.assertIsNone(details[0]["resolved"])
        self.assertIsNone(details[0]["resolved_sha256"])
        self.assertNotIn("BytesIO", json.dumps(details[0], sort_keys=True))
        self.assertNotIn("0x", json.dumps(details[0], sort_keys=True))

    def test_non_file_font_path_is_reported_as_null(self) -> None:
        class NonFileFont:
            path: str

        with tempfile.TemporaryDirectory() as temporary:
            font = NonFileFont()
            font.path = str(Path(temporary) / "missing-font.ttf")
            details = renderer.describe_font(
                font,  # type: ignore[arg-type]
                "missing-font.ttf",
            )

        self.assertIsNone(details["resolved"])
        self.assertIsNone(details["resolved_sha256"])
        self.assertTrue(details["fallback"])

    def test_relative_font_ignores_same_name_font_in_process_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            search_root = root / "task"
            search_root.mkdir()
            expected_font = search_root / "shared.ttf"
            expected_bytes = b"task-font"
            expected_font.write_bytes(expected_bytes)

            calling_dirs = [root / "caller-a", root / "caller-b"]
            for index, calling_dir in enumerate(calling_dirs):
                calling_dir.mkdir()
                (calling_dir / "shared.ttf").write_bytes(
                    f"wrong-cwd-font-{index}".encode()
                )

            original_cwd = Path.cwd()
            details_by_cwd = []
            try:
                with (
                    mock.patch.object(renderer, "FONT_SEARCH_DIRS", [search_root]),
                    mock.patch.object(
                        renderer.ImageFont,
                        "truetype",
                        side_effect=self._font_loader,
                    ),
                ):
                    for calling_dir in calling_dirs:
                        os.chdir(calling_dir)
                        font = renderer.find_font(24, "shared.ttf")
                        details_by_cwd.append(
                            renderer.describe_font(font, "shared.ttf")
                        )
            finally:
                os.chdir(original_cwd)

        self.assertEqual(
            [item["resolved"] for item in details_by_cwd],
            [str(expected_font.resolve())] * len(calling_dirs),
        )
        self.assertEqual(
            [item["resolved_sha256"] for item in details_by_cwd],
            [hashlib.sha256(expected_bytes).hexdigest()] * len(calling_dirs),
        )
        self.assertTrue(all(not item["fallback"] for item in details_by_cwd))
        self.assertTrue(
            all(item["pillow_version"] == PIL.__version__ for item in details_by_cwd)
        )

    def test_relative_subpath_requires_exact_match_in_search_dir_order(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first_root = root / "first"
            second_root = root / "second"
            (second_root / "fonts").mkdir(parents=True)
            first_root.mkdir()

            wrong_copy = first_root / "ui.ttf"
            wrong_copy.write_bytes(b"wrong-basename-copy")
            expected_font = second_root / "fonts" / "ui.ttf"
            expected_bytes = b"exact-relative-copy"
            expected_font.write_bytes(expected_bytes)

            with (
                mock.patch.object(
                    renderer, "FONT_SEARCH_DIRS", [first_root, second_root]
                ),
                mock.patch.object(
                    renderer.ImageFont,
                    "truetype",
                    side_effect=self._font_loader,
                ),
            ):
                font = renderer.find_font(24, "fonts/ui.ttf")
                details = renderer.describe_font(font, "fonts/ui.ttf")
                wrong_details = renderer.describe_font(
                    FontProbe(wrong_copy, "Probe", "Regular"),  # type: ignore[arg-type]
                    "fonts/ui.ttf",
                )

        self.assertEqual(details["resolved"], str(expected_font.resolve()))
        self.assertEqual(
            details["resolved_sha256"], hashlib.sha256(expected_bytes).hexdigest()
        )
        self.assertFalse(details["fallback"])
        self.assertTrue(wrong_details["fallback"])


class RendererStabilityTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.assets_dir = Path(self.temporary.name) / "assets"
        self.assets_dir.mkdir()

        self.auto_margins = {"left": 2, "top": 1, "right": 1, "bottom": 2}
        self._save_asset(
            "auto.png",
            painted_nine_regions((7, 6), self.auto_margins),
            meta_border=self.auto_margins,
        )

        one_pixel = {"left": 1, "top": 1, "right": 1, "bottom": 1}
        self._save_asset(
            "numeric.png",
            coordinate_pattern((5, 5)),
            meta_border={"left": 2, "top": 2, "right": 2, "bottom": 2},
        )

        self._save_asset(
            "clamped.png",
            painted_nine_regions(
                (6, 6), {"left": 2, "top": 2, "right": 2, "bottom": 2}
            ),
        )
        hue_probe = Image.new("RGBA", (3, 1))
        hue_probe.putdata([
            (255, 0, 0, 255),
            (0, 255, 0, 255),
            (0, 0, 255, 0),
        ])
        self._save_asset("hue.png", hue_probe)
        self.assets = renderer.AssetCache(self.assets_dir)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def _save_asset(
        self,
        name: str,
        image: Image.Image,
        *,
        meta_border: dict[str, int] | None = None,
    ) -> None:
        path = self.assets_dir / name
        image.save(path)
        if meta_border is not None:
            path.with_name(f"{path.name}.meta").write_text(
                "TextureImporter:\n"
                "  spriteBorder: "
                f"{{x: {meta_border['left']}, y: {meta_border['bottom']}, "
                f"z: {meta_border['right']}, w: {meta_border['top']}}}\n",
                encoding="utf-8",
            )

    @staticmethod
    def _structure(
        asset_name: str,
        target_size: tuple[int, int],
        nine_slice: object | None,
    ) -> dict:
        width, height = target_size
        root = {
            "type": "image",
            "name": "probe",
            "position": {"x": 0, "y": 0},
            "size": {"width": width, "height": height},
            "asset": asset_name,
        }
        if nine_slice is not None:
            root["nineSlice"] = nine_slice
        return {
            "canvas": {"width": width, "height": height},
            "root": root,
        }

    def _render(
        self,
        asset_name: str,
        target_size: tuple[int, int],
        nine_slice: object | None,
        trace: list[dict] | None = None,
    ) -> Image.Image:
        return renderer.render_from_structure(
            self._structure(asset_name, target_size, nine_slice),
            self.assets,
            (0, 0, 0, 0),
            trace,
        )

    def assert_pixel_golden(
        self, actual: Image.Image, expected: Image.Image
    ) -> None:
        self.assertEqual(actual.mode, expected.mode)
        self.assertEqual(actual.size, expected.size)
        actual_hash = hashlib.sha256(actual.tobytes()).hexdigest()
        expected_hash = hashlib.sha256(expected.tobytes()).hexdigest()
        self.assertEqual(actual_hash, expected_hash)

    def test_boolean_auto_uses_metadata_border_pixel_golden(self) -> None:
        target_size = (11, 10)
        trace: list[dict] = []

        actual = self._render("auto.png", target_size, True, trace)
        expected = painted_nine_regions(target_size, self.auto_margins)

        self.assert_pixel_golden(actual, expected)
        self.assertEqual(trace[0]["asset"]["nine_slice_margins"], self.auto_margins)
        self.assertIsNone(trace[0]["asset"]["render_error"])

    def test_numeric_one_is_one_pixel_not_boolean_auto(self) -> None:
        target_size = (9, 7)
        margins = {"left": 1, "top": 1, "right": 1, "bottom": 1}
        expected = self._render("numeric.png", target_size, margins)

        for value in (1, 1.0):
            with self.subTest(value=value):
                trace: list[dict] = []
                actual = self._render("numeric.png", target_size, value, trace)

                self.assert_pixel_golden(actual, expected)
                self.assertEqual(
                    trace[0]["asset"]["nine_slice_margins"], margins
                )
                self.assertIsNone(trace[0]["asset"]["render_error"])

    def test_explicit_margins_trace_records_clamped_effective_pixel_golden(self) -> None:
        requested = {"left": 4, "top": 4, "right": 3, "bottom": 3}
        effective = {"left": 2, "top": 2, "right": 2, "bottom": 2}
        target_size = (5, 5)
        trace: list[dict] = []

        actual = self._render("clamped.png", target_size, requested, trace)
        expected = painted_nine_regions(target_size, effective)

        self.assert_pixel_golden(actual, expected)
        self.assertEqual(trace[0]["asset"]["nine_slice_margins"], effective)
        self.assertIsNone(trace[0]["asset"]["render_error"])

    def test_hue_shift_rotates_visible_pixels_and_preserves_alpha(self) -> None:
        source = Image.open(self.assets_dir / "hue.png").convert("RGBA")

        actual = renderer.apply_hue_shift(source, 120)
        expected = Image.new("RGBA", (3, 1))
        expected.putdata([
            (0, 255, 0, 255),
            (0, 0, 255, 255),
            (0, 0, 255, 0),
        ])

        self.assert_pixel_golden(actual, expected)
        self.assert_pixel_golden(renderer.apply_hue_shift(source, 0), source)

    def test_render_trace_records_hue_shift(self) -> None:
        structure = self._structure("hue.png", (3, 1), None)
        structure["root"]["hueShift"] = -75
        trace: list[dict] = []

        renderer.render_from_structure(
            structure, self.assets, (0, 0, 0, 0), trace
        )

        self.assertEqual(trace[0]["asset"]["hue_shift"], -75)
        self.assertIsNone(trace[0]["asset"]["render_error"])

    def test_stretch_resize_error_is_traced_and_raised(self) -> None:
        trace: list[dict] = []
        self.assets.get("numeric.png")

        with mock.patch.object(
            Image.Image, "resize", side_effect=OSError("resize exploded")
        ):
            with self.assertRaisesRegex(
                RuntimeError,
                r"stretch render failed for asset 'numeric\.png' at root: "
                r"OSError: resize exploded",
            ):
                self._render("numeric.png", (9, 7), None, trace)

        self.assertEqual(len(trace), 1)
        self.assertEqual(trace[0]["asset"]["render_error"], "OSError: resize exploded")

    def test_nine_slice_error_is_traced_and_raised(self) -> None:
        trace: list[dict] = []

        with mock.patch.object(
            renderer,
            "render_nine_slice",
            side_effect=ValueError("nine-slice exploded"),
        ):
            with self.assertRaisesRegex(
                RuntimeError,
                r"nine-slice render failed for asset 'auto\.png' at root: "
                r"ValueError: nine-slice exploded",
            ):
                self._render("auto.png", (11, 10), True, trace)

        self.assertEqual(len(trace), 1)
        self.assertEqual(
            trace[0]["asset"]["render_error"],
            "ValueError: nine-slice exploded",
        )


if __name__ == "__main__":
    unittest.main()
