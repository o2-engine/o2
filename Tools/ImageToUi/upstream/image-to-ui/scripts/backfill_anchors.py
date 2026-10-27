"""Add required anchor-alignment metadata to every UI structure node.

Anchors describe which parent reference point a node is attached to. They do
not change the existing top-left ``position`` coordinates or layout behavior.
Missing axes are inferred from each node's resolved box by choosing the nearest
left/center/right and top/middle/bottom reference in its parent.

Usage:
    py -B backfill_anchors.py \
        --structure ui_structure.json \
        --output ui_structure.with-anchors.json

Pass the same path to ``--structure`` and ``--output`` for an atomic in-place
upgrade. Existing valid anchor values are preserved unless ``--overwrite`` is
used.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
from typing import Any

import layout as layout_resolver


HORIZONTAL_ANCHORS = ("left", "center", "right")
VERTICAL_ANCHORS = ("top", "middle", "bottom")
ANCHOR_FIELDS = {"horizontal", "vertical"}


class AnchorBackfillError(ValueError):
    pass


def _nearest_anchor(
    position: float,
    extent: float,
    parent_extent: float,
    names: tuple[str, str, str],
    prefer_center: bool = True,
) -> str:
    candidates = (
        (names[0], 0.0),
        (names[1], (parent_extent - extent) / 2.0),
        (names[2], parent_extent - extent),
    )
    # A full-parent image has identical distances to all three references.
    # Prefer the center on ties; attachment does not imply stretching.
    return min(candidates, key=lambda candidate: (
        abs(position - candidate[1]), prefer_center and candidate[0] != names[1]
    ))[0]


def _horizontal_hint(value: Any) -> str | None:
    if value in ("left", "start"):
        return "left"
    if value in ("center", "middle"):
        return "center"
    if value in ("right", "end"):
        return "right"
    return None


def _vertical_hint(value: Any) -> str | None:
    if value in ("top", "start"):
        return "top"
    if value in ("middle", "center"):
        return "middle"
    if value in ("bottom", "end"):
        return "bottom"
    return None


def _inferred_anchor(
    node: dict[str, Any],
    parent_size: tuple[float, float],
    parent_layout: dict[str, Any] | None,
    prefer_center: bool = True,
) -> dict[str, str]:
    relative = node.get("_rel")
    if not isinstance(relative, list) or len(relative) != 4:
        raise AnchorBackfillError("resolved node is missing its relative box")
    x, y, width, height = (float(value) for value in relative)
    parent_width, parent_height = parent_size
    inferred = {
        "horizontal": _nearest_anchor(
            x,
            width,
            parent_width,
            HORIZONTAL_ANCHORS,
            prefer_center,
        ),
        "vertical": _nearest_anchor(
            y,
            height,
            parent_height,
            VERTICAL_ANCHORS,
            prefer_center,
        ),
    }
    responsive = node.get("responsive")
    if responsive:
        for field, axis, names in (
            ("horizontal", "x", HORIZONTAL_ANCHORS),
            ("vertical", "y", VERTICAL_ANCHORS),
        ):
            center = (responsive["min"][axis] + responsive["max"][axis]) / 2
            inferred[field] = min(zip(names, (0, 0.5, 1)), key=lambda item: abs(item[1]-center))[0]
        return inferred
    layout_type = parent_layout.get("type") if parent_layout else None
    if layout_type == "row":
        vertical = _vertical_hint(
            node.get("vAlign", parent_layout.get("vAlign"))
        )
        if vertical is not None:
            inferred["vertical"] = vertical
    elif layout_type == "column":
        horizontal = _horizontal_hint(
            node.get("align", parent_layout.get("vAlign"))
        )
        if horizontal is not None:
            inferred["horizontal"] = horizontal
    else:
        horizontal = _horizontal_hint(node.get("align"))
        vertical = _vertical_hint(node.get("vAlign"))
        if horizontal is not None:
            inferred["horizontal"] = horizontal
        if vertical is not None:
            inferred["vertical"] = vertical
    return inferred


def _checked_existing_anchor(
    node: dict[str, Any],
    path: str,
) -> dict[str, str] | None:
    if "anchor" not in node:
        return None
    anchor = node["anchor"]
    if not isinstance(anchor, dict):
        raise AnchorBackfillError(f"{path}.anchor must be an object")
    unknown = sorted(set(anchor) - ANCHOR_FIELDS)
    if unknown:
        raise AnchorBackfillError(
            f"{path}.anchor has unknown fields: {', '.join(unknown)}"
        )
    allowed_by_field = {
        "horizontal": HORIZONTAL_ANCHORS,
        "vertical": VERTICAL_ANCHORS,
    }
    for field, allowed in allowed_by_field.items():
        if field in anchor and anchor[field] not in allowed:
            raise AnchorBackfillError(
                f"{path}.anchor.{field} must be one of {list(allowed)}"
            )
    return dict(anchor)


def _place_anchor(node: dict[str, Any], anchor: dict[str, str]) -> None:
    ordered: dict[str, Any] = {}
    inserted = False
    for key, value in node.items():
        if key == "anchor":
            continue
        ordered[key] = value
        if key == "size":
            ordered["anchor"] = anchor
            inserted = True
    if not inserted:
        ordered["anchor"] = anchor
    node.clear()
    node.update(ordered)


def backfill_anchors(
    structure: dict[str, Any],
    *,
    overwrite: bool = False,
) -> tuple[dict[str, Any], dict[str, int]]:
    """Return a deep-copied structure with complete anchors on every node."""
    root = structure.get("root")
    if not isinstance(root, dict):
        raise AnchorBackfillError("structure.root must be an object")
    canvas = structure.get("canvas")
    if not isinstance(canvas, dict):
        raise AnchorBackfillError("structure.canvas must be an object")
    try:
        canvas_size = (float(canvas["width"]), float(canvas["height"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise AnchorBackfillError(
            "structure.canvas.width and height must be numbers"
        ) from exc

    output = copy.deepcopy(structure)
    resolved = layout_resolver.resolve_positions(copy.deepcopy(structure))
    stats = {"total": 0, "added": 0, "updated": 0, "unchanged": 0}

    def walk(
        authored_node: dict[str, Any],
        resolved_node: dict[str, Any],
        parent_size: tuple[float, float],
        parent_layout: dict[str, Any] | None,
        path: str,
    ) -> None:
        stats["total"] += 1
        inferred = _inferred_anchor(
            resolved_node,
            parent_size,
            parent_layout,
            prefer_center=path != "root",
        )

        if overwrite:
            had_anchor = "anchor" in authored_node
            anchor = inferred
            status = "updated" if had_anchor else "added"
        else:
            existing = _checked_existing_anchor(authored_node, path)
            if existing is None:
                anchor = inferred
                status = "added"
            else:
                anchor = {
                    "horizontal": existing.get(
                        "horizontal", inferred["horizontal"]
                    ),
                    "vertical": existing.get("vertical", inferred["vertical"]),
                }
                status = "unchanged" if len(existing) == 2 else "updated"

        if status != "unchanged":
            _place_anchor(authored_node, anchor)
        stats[status] += 1

        authored_children = authored_node.get("children") or []
        resolved_children = resolved_node.get("children") or []
        if not isinstance(authored_children, list) or not isinstance(
            resolved_children, list
        ):
            raise AnchorBackfillError(f"{path}.children must be a list")
        if len(authored_children) != len(resolved_children):
            raise AnchorBackfillError(f"{path}.children changed during resolution")

        relative = resolved_node.get("_rel") or [0, 0, 0, 0]
        node_size = (float(relative[2]), float(relative[3]))
        for index, (authored_child, resolved_child) in enumerate(
            zip(authored_children, resolved_children)
        ):
            if not isinstance(authored_child, dict) or not isinstance(
                resolved_child, dict
            ):
                raise AnchorBackfillError(
                    f"{path}.children[{index}] must be an object"
                )
            name = authored_child.get("name", "?")
            walk(
                authored_child,
                resolved_child,
                node_size,
                authored_node.get("layout"),
                f"{path}/{name}",
            )

    walk(output["root"], resolved["root"], canvas_size, None, "root")
    return output, stats


def _read_structure(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AnchorBackfillError(f"invalid structure JSON: {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise AnchorBackfillError("structure root must be an object")
    return value


def _write_json_atomic(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.tmp")
    temporary.write_text(
        json.dumps(value, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    temporary.replace(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--structure", required=True, help="Input ui_structure.json")
    parser.add_argument(
        "--output",
        required=True,
        help="Output JSON; may equal --structure",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Recompute existing anchors as well as filling missing values",
    )
    args = parser.parse_args()

    source = Path(args.structure).expanduser().resolve()
    destination = Path(args.output).expanduser().resolve()
    try:
        result, stats = backfill_anchors(
            _read_structure(source),
            overwrite=args.overwrite,
        )
        _write_json_atomic(destination, result)
    except AnchorBackfillError as exc:
        raise SystemExit(f"Anchor backfill error: {exc}") from exc

    print(f"Anchors written: {destination}")
    print(
        "  nodes: {total}; added: {added}; updated: {updated}; "
        "unchanged: {unchanged}".format(**stats)
    )


if __name__ == "__main__":
    main()
