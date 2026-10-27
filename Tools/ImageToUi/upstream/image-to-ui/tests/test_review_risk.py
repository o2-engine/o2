from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageDraw


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))
import review_risk  # noqa: E402


def trace_for(
    bbox: list[int],
    *,
    element_type: str = "rect",
    text: dict | None = None,
) -> dict:
    item = {
        "path": "root/probe",
        "name": "probe",
        "type": element_type,
        "bbox": bbox,
        "visible_bbox": bbox,
        "z_order": 10,
    }
    if text is not None:
        item["text"] = text
    return {
        "version": 1,
        "canvas": {"width": 64, "height": 64},
        "elements": [item],
    }


class ReviewRiskTests(unittest.TestCase):
    def test_exact_reconstruction_has_zero_residual(self) -> None:
        design = Image.new("RGB", (64, 64), "black")
        ImageDraw.Draw(design).rectangle((8, 8, 47, 47), fill="white")
        reconstruction = design.convert("RGBA")

        items = review_risk.analyze(
            trace_for([8, 8, 40, 40]), design, reconstruction
        )

        self.assertEqual(len(items), 1)
        self.assertEqual(items[0]["residual"]["score"], 0.0)
        self.assertNotEqual(items[0]["risk_level"], "high")

    def test_salient_mismatch_is_high_and_gets_focused_evidence(self) -> None:
        design = Image.new("RGB", (64, 64), "black")
        reconstruction = Image.new("RGBA", (64, 64), (0, 0, 0, 255))
        ImageDraw.Draw(reconstruction).rectangle(
            (8, 8, 47, 47), fill=(255, 255, 255, 255)
        )
        items = review_risk.analyze(
            trace_for([8, 8, 40, 40]), design, reconstruction
        )
        self.assertEqual(items[0]["risk_level"], "high")
        self.assertGreater(items[0]["salience"]["score"], 0.6)
        self.assertEqual(items[0]["residual"]["score"], 1.0)

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            evidence = root / "risk_review.png"
            legend = root / "risk_review_legend.json"
            covered = review_risk.build_evidence(
                design, reconstruction, items, evidence, legend
            )

            self.assertEqual(covered, {"root/probe"})
            self.assertTrue(evidence.is_file())
            legend_value = json.loads(legend.read_text(encoding="utf-8"))
            self.assertEqual(
                legend_value["targets"][0]["path"], "root/probe"
            )

    def test_transparent_omission_does_not_create_false_residual(self) -> None:
        design = Image.new("RGB", (64, 64), (35, 70, 105))
        reconstruction = Image.new("RGBA", (64, 64), (0, 0, 0, 0))

        items = review_risk.analyze(
            trace_for([8, 8, 40, 40]), design, reconstruction
        )

        self.assertEqual(items[0]["residual"]["score"], 0.0)
        self.assertNotEqual(items[0]["risk_level"], "high")

    def test_large_text_contributes_to_salience_without_name_heuristics(self) -> None:
        design = Image.new("RGB", (64, 64), "black")
        reconstruction = design.convert("RGBA")
        items = review_risk.analyze(
            trace_for(
                [20, 20, 24, 20],
                element_type="text",
                text={"font_size": 12, "line_ink_bboxes": [[20, 20, 24, 20]]},
            ),
            design,
            reconstruction,
        )

        self.assertGreaterEqual(items[0]["salience"]["font_score"], 1.0)
        self.assertIn(
            "salience.large-text",
            items[0]["reasons"],
        )


if __name__ == "__main__":
    unittest.main()
