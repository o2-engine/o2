"""
Shared layout / alignment resolver.

Every script that needs to know the absolute position of every element
(annotate_grid, annotate_element, render_comparison) calls
`resolve_positions(structure)` once, then reads each element's `_abs`
field for its computed absolute (x, y, w, h) in canvas coordinates.

Position decision order for a child:
  1. Parent has `layout` → layout controls the main axis
     - row:    x comes from layout; child `vAlign` may override cross-axis y
     - column: y comes from layout; child `align` may override cross-axis x
  2. No parent layout, child has `align` or `vAlign` → derived from parent
  3. No parent layout, child has explicit `position` → use as-is
  4. Otherwise                                      → (0, 0)

Supported layout types:
  - "row" / "column"  — linear distribution along axis

Layout fields (all optional):
  - type:    "row" | "column"            (required if layout present)
  - spacing: number (fixed px) | "even"  (auto-distribute leftover space)
  - padding: { x: number, y: number }
  - align:   "start" | "center" | "end" | "space-between" | "space-around"
             (main axis)
  - vAlign:  "start" | "middle" | "end"  (cross axis)

Element-level alignment:
  - align:   "left" | "center" | "right"
  - vAlign:  "top"  | "middle" | "bottom"
  - offset:  { x: number, y: number }     — micro-adjustment after alignment

Within a layout group, element-level alignment only affects the cross axis.
Use `offset` for small per-child adjustments; remove the parent layout when a
child needs a fully independent position.

Backwards compatibility:
  - Required `anchor` metadata describes attachment intent but does not alter
    bbox resolution; `position` remains parent-relative top-left coordinates.
  - Optional `responsive` normalized edges and pixel offsets drive actual size
    adaptation. They cannot compete with a parent layout or child alignment.
  - Scroll content is translated and annotated with its viewport clip.
  - Elements without layout/align/responsive use explicit `position` as before.
  - This module never deletes or overwrites authored fields; it only annotates
    the tree with `_abs` for downstream consumers.
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple


import ui_components as components


LAYOUT_TYPES = {"row", "column", "grid"}


def _to_int(x: Any, default: int = 0) -> int:
    try:
        return int(round(float(x)))
    except (TypeError, ValueError):
        return default


def _get_size(elem: dict) -> Tuple[int, int]:
    size = elem.get("size") or {}
    return _to_int(size.get("width")), _to_int(size.get("height"))


def _get_position(elem: dict) -> Tuple[int, int]:
    pos = elem.get("position") or {}
    return _to_int(pos.get("x")), _to_int(pos.get("y"))


def _get_offset(elem: dict) -> Tuple[int, int]:
    off = elem.get("offset") or {}
    return _to_int(off.get("x")), _to_int(off.get("y"))


def _get_padding(layout: dict) -> Tuple[int, int]:
    pad = layout.get("padding")
    if isinstance(pad, (int, float)):
        v = _to_int(pad)
        return v, v
    if isinstance(pad, dict):
        return _to_int(pad.get("x")), _to_int(pad.get("y"))
    return 0, 0


def _children(elem: dict) -> List[dict]:
    return elem.get("children") or []


def _layout_type(layout: Any) -> str:
    if not isinstance(layout, dict):
        raise ValueError("layout must be an object with an explicit layout.type")
    layout_type = layout.get("type")
    if not isinstance(layout_type, str) or layout_type not in LAYOUT_TYPES:
        raise ValueError("layout.type must be explicitly set to row or column or grid")
    return layout_type


def _distribute_pixels(total: int, slots: int) -> List[int]:
    """Split integer pixels across slots without dropping a remainder.

    Cumulative integer boundaries keep the result deterministic, consume the
    exact total, and make any two slots differ by at most one pixel. The floor
    at the leading boundary gives odd centered layouts their existing one-pixel
    bias toward the trailing edge.
    """
    if slots <= 0:
        return []
    return [
        ((index + 1) * total) // slots - (index * total) // slots
        for index in range(slots)
    ]


# ---------------------------------------------------------------------------
# Per-element alignment within a parent box (no layout group involved).
# ---------------------------------------------------------------------------

def _align_axis(parent_extent: int, elem_extent: int, align: str,
                explicit: Optional[int]) -> int:
    """Return the start coordinate of `elem_extent` inside `parent_extent`
    given an alignment keyword. If `align` is not recognized, fall back to
    `explicit` (the authored position) or 0.
    """
    if align in ("start", "left", "top"):
        return 0
    if align in ("center", "middle"):
        return (parent_extent - elem_extent) // 2
    if align in ("end", "right", "bottom"):
        return parent_extent - elem_extent
    return explicit if explicit is not None else 0


def _resolve_alignment(elem: dict, parent_w: int, parent_h: int
                       ) -> Tuple[int, int]:
    """Resolve an element's (x, y) inside its parent given align/vAlign +
    offset. Falls back to the element's authored position for any axis
    that isn't aligned.
    """
    ew, eh = _get_size(elem)
    ax = elem.get("align")
    ay = elem.get("vAlign")
    px, py = _get_position(elem)
    ox, oy = _get_offset(elem)

    if ax is not None:
        x = _align_axis(parent_w, ew, ax, px) + ox
    else:
        x = px + ox
    if ay is not None:
        y = _align_axis(parent_h, eh, ay, py) + oy
    else:
        y = py + oy
    return x, y


# ---------------------------------------------------------------------------
# Layout group resolution (row / column).
# ---------------------------------------------------------------------------

def _layout_group(parent: dict, layout: dict, parent_size=None) -> List[Tuple[int, int]]:
    """Compute (x, y) for each child of `parent` according to `layout`.
    Returns a list aligned with `_children(parent)`.

    Coordinate space: relative to parent's top-left (inside padding).
    """
    ltype = _layout_type(layout)
    children = _children(parent)
    if not children:
        return []

    pw, ph = parent_size or _get_size(parent)
    pad_x, pad_y = _get_padding(layout)
    inner_w = max(0, pw - 2 * pad_x)
    inner_h = max(0, ph - 2 * pad_y)

    main_axis = "x" if ltype == "row" else "y"
    main_extent = inner_w if main_axis == "x" else inner_h
    cross_extent = inner_h if main_axis == "x" else inner_w

    sizes_main = []
    sizes_cross = []
    for c in children:
        cw, ch = _get_size(c)
        if main_axis == "x":
            sizes_main.append(cw); sizes_cross.append(ch)
        else:
            sizes_main.append(ch); sizes_cross.append(cw)

    if ltype == "grid":
        columns = layout.get("columns")
        cell = layout.get("cellSize") or {}
        if isinstance(columns, bool) or not isinstance(columns, int) or columns <= 0:
            raise ValueError("grid columns must be a positive integer")
        cw, ch = _to_int(cell.get("width")), _to_int(cell.get("height"))
        if cw <= 0 or ch <= 0:
            raise ValueError("grid cellSize must be positive")
        spacing = layout.get("spacing", {})
        if not isinstance(spacing, dict):
            raise ValueError("grid spacing must be {x,y}")
        gx, gy = _to_int(spacing.get("x")), _to_int(spacing.get("y"))
        used_columns = min(columns, len(children))
        rows = (len(children) + columns - 1) // columns
        width = used_columns*cw + max(0, used_columns-1)*gx
        height = rows*ch + max(0, rows-1)*gy
        x = pad_x + _align_axis(inner_w, width, layout.get("align", "start"), 0)
        y = pad_y + _align_axis(inner_h, height, layout.get("vAlign", "start"), 0)
        return [(x+(i%columns)*(cw+gx)+_get_offset(c)[0],
                 y+(i//columns)*(ch+gy)+_get_offset(c)[1]) for i,c in enumerate(children)]

    n = len(children)
    total_main = sum(sizes_main)
    leftover = max(0, main_extent - total_main)

    spacing_cfg = layout.get("spacing", 0)
    align = layout.get("align", "start")
    v_align = layout.get("vAlign", "start")

    # Determine per-gap spacing and leading offset.
    distribution = align if align in (
        "space-between", "space-around", "space-evenly"
    ) else ("space-evenly" if spacing_cfg == "even" else None)
    if distribution == "space-between":
        lead = 0
        gaps = _distribute_pixels(leftover, n - 1)
    elif distribution == "space-around":
        half_gaps = _distribute_pixels(leftover, 2 * n)
        lead = half_gaps[0]
        gaps = [
            half_gaps[2 * index + 1] + half_gaps[2 * index + 2]
            for index in range(n - 1)
        ]
    elif distribution == "space-evenly":
        all_gaps = _distribute_pixels(leftover, n + 1)
        lead = all_gaps[0]
        gaps = all_gaps[1:-1]
    else:
        # Fixed spacing
        fixed = _to_int(spacing_cfg)
        total_with_gaps = total_main + fixed * max(0, n - 1)
        if align in ("center", "middle"):
            lead = max(0, (main_extent - total_with_gaps) // 2)
        elif align in ("end", "right", "bottom"):
            lead = max(0, main_extent - total_with_gaps)
        else:  # start
            lead = 0
        gaps = [fixed] * (n - 1)

    # Walk children, accumulate main-axis position
    positions: List[Tuple[int, int]] = []
    cursor = lead
    for i, c in enumerate(children):
        # Cross-axis position
        cross = _align_axis(cross_extent, sizes_cross[i], v_align, 0)
        if main_axis == "x":
            x = pad_x + cursor
            y = pad_y + cross
        else:
            x = pad_x + cross
            y = pad_y + cursor
        # Child-level alignment can override only the cross axis. The layout
        # still owns the main axis so a row/column cannot collapse because a
        # child uses vAlign/align for centering.
        if main_axis == "x":
            child_cross = c.get("vAlign")
            if child_cross is not None:
                y = pad_y + _align_axis(cross_extent, sizes_cross[i], child_cross, cross)
        else:
            child_cross = c.get("align")
            if child_cross is not None:
                x = pad_x + _align_axis(cross_extent, sizes_cross[i], child_cross, cross)

        ox, oy = _get_offset(c)
        positions.append((x + ox, y + oy))
        cursor += sizes_main[i]
        if i < n - 1:
            cursor += gaps[i]
    return positions


# ---------------------------------------------------------------------------
# Public entry points.
# ---------------------------------------------------------------------------

def resolve_positions(structure: dict) -> dict:
    """Resolve authored geometry and optional responsive/scroll constraints."""
    root = structure.get("root", {})
    canvas = structure.get("canvas") or {}
    for _, node in components.nodes(root):
        for field in components.RUNTIME_FIELDS:
            node.pop(field, None)

    def walk(elem, ax, ay, pw, ph):
        children = _children(elem)
        has_layout = "layout" in elem
        positions = None
        if has_layout:
            _layout_type(elem["layout"])
            positions = _layout_group(elem, elem["layout"], (pw, ph))
        for index, child in enumerate(children):
            cw, ch = _get_size(child)
            if "responsive" in child:
                rx, ry, cw, ch = components.responsive_box(
                    child["responsive"], pw, ph, canvas.get("safeArea", {}),
                )
            elif positions is not None:
                rx, ry = positions[index]
            else:
                rx, ry = _resolve_alignment(child, pw, ph)
            scroll = elem.get("scroll")
            if scroll and child.get("name") == scroll["content"]:
                if rx != 0 or ry != 0:
                    raise components.ComponentError("scroll content must start at viewport (0, 0)")
                extent_x, extent_y = max(0, cw-pw), max(0, ch-ph)
                desired = scroll.get("offset", {})
                sx = min(extent_x, max(0, _to_int(desired.get("x"))))
                sy = min(extent_y, max(0, _to_int(desired.get("y"))))
                rx -= sx
                ry -= sy
                child["_scroll_content"] = True
                child["_clip_bbox"] = [ax, ay, pw, ph]
                elem["_scroll_metrics"] = {
                    "offset": {"x": sx, "y": sy},
                    "extent": {"x": extent_x, "y": extent_y},
                }
            child["_rel"] = [rx, ry, cw, ch]
            child["_abs"] = [ax+rx, ay+ry, cw, ch]
            walk(child, ax+rx, ay+ry, cw, ch)

    rw, rh = _get_size(root)
    rw = rw or _to_int(canvas.get("width"))
    rh = rh or _to_int(canvas.get("height"))
    rx, ry = _get_position(root)
    root["_rel"] = [rx, ry, rw, rh]
    root["_abs"] = [rx, ry, rw, rh]
    walk(root, rx, ry, rw, rh)
    components.apply_progress_clips(structure)
    return structure


def find_by_path(structure: dict, path: str) -> Optional[dict]:
    """Look up an element by slash-separated name path, e.g.
    'root/level_popup/popup_header/close_button'. Names are matched by the
    `name` field of each element. Returns the element dict, or None.
    """
    parts = [p for p in path.split("/") if p]
    if not parts:
        return None
    node = structure.get("root", {})
    if node.get("name") != parts[0]:
        return None
    for part in parts[1:]:
        next_node = None
        for c in _children(node):
            if c.get("name") == part:
                next_node = c
                break
        if next_node is None:
            return None
        node = next_node
    return node


def dfs_paths(structure: dict, include_root: bool = False) -> List[str]:
    """Return every element's path in DFS pre-order. Useful for driving the
    alignment loop top-down (parent before children).
    """
    out: List[str] = []

    def walk(elem: dict, prefix: str):
        name = elem.get("name", "")
        path = f"{prefix}/{name}" if prefix else name
        if prefix or include_root:
            out.append(path)
        else:
            # First call: root itself; only add if include_root
            if include_root:
                out.append(path)
        for c in _children(elem):
            walk(c, path)

    root = structure.get("root", {})
    walk(root, "")
    if include_root and (not out or out[0] != root.get("name")):
        out.insert(0, root.get("name", "root"))
    return out
