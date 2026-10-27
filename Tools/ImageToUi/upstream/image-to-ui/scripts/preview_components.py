"""Render one component scenario without replacing the native-design evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import audit_render
import render_comparison as renderer
import ui_components as components
from validate_structure import validate_structure


def assignments(values, convert):
    result = {}
    for item in values or []:
        path, separator, value = item.partition("=")
        if not separator or not path or path in result:
            raise components.ComponentError("Use unique PATH=VALUE assignments.")
        result[path] = convert(value)
    return result


def scroll_offset(value):
    parts = value.split(",")
    if len(parts) != 2:
        raise components.ComponentError("Scroll assignment must be PATH=X,Y.")
    return {"x": float(parts[0]), "y": float(parts[1])}


def render_preview(structure_path: Path, assets_dir: Path, design_path: Path,
                   inventory_path: Path, output: Path, *, size=None, states=None,
                   progress=None, scroll=None, transparent=False) -> dict:
    output.mkdir(parents=True, exist_ok=True)
    for name in ("reconstruction.png", "render_trace.json", "preview_report.json"):
        (output / name).unlink(missing_ok=True)
    source_bytes = structure_path.read_bytes()
    original = json.loads(source_bytes)
    # Validate authoring and all variant assets before constructing a scenario.
    baseline, _ = validate_structure(original, design_path, assets_dir)
    if baseline.errors or baseline.warnings:
        raise components.ComponentError("; ".join(baseline.errors + baseline.warnings))
    scene = components.snapshot(original, size=size, states=states, progress=progress, scroll=scroll)
    validation, stats = validate_structure(scene, None, assets_dir)
    if validation.errors or validation.warnings:
        raise components.ComponentError("; ".join(validation.errors + validation.warnings))
    renderer.FONT_SEARCH_DIRS = [path.resolve() for path in (
        structure_path.parent, structure_path.parent.parent,
        assets_dir, assets_dir.parent, design_path.parent, design_path.parent.parent,
    )]
    background = (0, 0, 0, 0) if transparent else (30, 30, 40, 255)
    entries = []
    reconstruction = renderer.render_from_structure(scene, renderer.AssetCache(assets_dir), background, entries)
    reconstruction.save(output / "reconstruction.png")
    trace = {
        "version": 1,
        "canvas": {"width": reconstruction.width, "height": reconstruction.height},
        "elements": entries,
    }
    (output / "render_trace.json").write_text(json.dumps(trace, indent=2) + "\n", encoding="utf-8")
    inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    audit = audit_render.audit_trace(trace, inventory)
    report = {
        "valid": audit["valid"],
        "purpose": "Behavior/geometry preview; not a match against an unseen design.",
        "structure_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "scenario": {"size": list(reconstruction.size), "states": states or {},
                     "progress": progress or {}, "scroll": scroll or {}},
        "validation": {"errors": validation.errors, "warnings": validation.warnings, "stats": stats},
        "visual_audit": audit,
    }
    if structure_path.read_bytes() != source_bytes:
        raise components.ComponentError("Structure changed during preview; rerun check.")
    (output / "preview_report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("structure", "assets", "design", "inventory", "output"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--size", nargs=2, type=int, metavar=("WIDTH", "HEIGHT"))
    parser.add_argument("--state", action="append")
    parser.add_argument("--progress", action="append")
    parser.add_argument("--scroll", action="append")
    parser.add_argument("--transparent-bg", action="store_true")
    args = parser.parse_args()
    try:
        report = render_preview(
            Path(args.structure), Path(args.assets), Path(args.design), Path(args.inventory), Path(args.output),
            size=args.size, states=assignments(args.state, str),
            progress=assignments(args.progress, float), scroll=assignments(args.scroll, scroll_offset),
            transparent=args.transparent_bg,
        )
    except (ValueError, OSError, renderer.RenderError) as exc:
        for name in ("reconstruction.png", "render_trace.json", "preview_report.json"):
            (Path(args.output) / name).unlink(missing_ok=True)
        parser.exit(1, f"Preview failed: {exc}\n")
    print(f"Preview artifacts: {args.output}")
    if not report["valid"]:
        parser.exit(1, "Preview visual audit failed; inspect preview_report.json.\n")


if __name__ == "__main__":
    main()
