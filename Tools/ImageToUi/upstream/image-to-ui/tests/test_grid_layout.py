import copy
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import layout
import validate_structure
import scale_structure


def document():
    return {"canvas": {"width": 200, "height": 120}, "root": {
        "type": "container", "name": "root", "position": {"x": 0, "y": 0},
        "size": {"width": 200, "height": 120},
        "anchor": {"horizontal": "left", "vertical": "top"},
        "layout": {"type": "grid", "columns": 2, "cellSize": {"width": 40, "height": 20},
                   "spacing": {"x": 8, "y": 6}, "padding": {"x": 10, "y": 5}},
        "children": [{"type": "rect", "name": str(i), "color": "#FFFFFF",
                      "size": {"width": 40, "height": 20},
                      "anchor": {"horizontal": "left", "vertical": "top"}}
                     for i in range(5)]}}


class GridLayoutTests(unittest.TestCase):
    def validate(self, s):
        with tempfile.TemporaryDirectory() as path:
            return validate_structure.validate_structure(s, None, Path(path))[0]

    def test_grid_places_rows_and_incomplete_last_row(self):
        s = document()
        self.assertEqual(self.validate(s).errors, [])
        root = layout.resolve_positions(s)["root"]
        self.assertEqual([n["_rel"] for n in root["children"]],
                         [[10,5,40,20], [58,5,40,20], [10,31,40,20], [58,31,40,20], [10,57,40,20]])

    def test_grid_alignment_and_cell_size_survive_scaling(self):
        s = document(); s["root"]["layout"].update(align="center", vAlign="bottom")
        root = layout.resolve_positions(copy.deepcopy(s))["root"]
        self.assertEqual(root["children"][0]["_rel"], [56,43,40,20])
        scaled = scale_structure.scale_structure(s, 2, 3)
        grid = scaled["root"]["layout"]
        self.assertEqual(grid["cellSize"], {"width":80,"height":60})
        self.assertEqual(grid["spacing"], {"x":16,"y":18})
        self.assertEqual(grid["columns"], 2)

    def test_invalid_grid_contracts_are_rejected(self):
        for field, value in [("columns", 0), ("columns", 1.5), ("columns", True),
                             ("cellSize", {"width":0,"height":20}), ("spacing", "even"),
                             ("spacing", {"x":float("nan"),"y":0}), ("align", "space-between")]:
            with self.subTest(field=field, value=value):
                s=document(); s["root"]["layout"][field]=value
                self.assertTrue(self.validate(s).errors)

    def test_grid_cells_must_match_and_cannot_have_independent_alignment(self):
        for field, value in [("size", {"width":30,"height":20}), ("align", "center")]:
            s=document(); s["root"]["children"][0][field]=value
            self.assertTrue(self.validate(s).errors)
