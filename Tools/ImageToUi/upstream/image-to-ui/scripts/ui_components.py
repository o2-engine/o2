"""Validated UI behavior descriptions and deterministic preview snapshots.

Coordinates use the existing top-left origin. Authored fields are preserved;
state and progress snapshots are deep copies, and geometry diagnostics use
private fields on the layout-resolved tree.
"""

from __future__ import annotations

import copy
import math
import string
from typing import Any


class ComponentError(ValueError):
    pass


STATE_FIELDS = {
    "asset", "color", "opacity", "hueShift", "nineSlice", "text", "fontFamily",
    "fontSize", "lineHeight", "textScaleX", "strokeColor", "strokeWidth",
    "alignment", "textVAlign", "visible",
}
DIRECTIONS = {"left-to-right", "right-to-left", "bottom-to-top", "top-to-bottom"}
RUNTIME_FIELDS = {"_clip_bbox", "_scroll_content", "_scroll_metrics", "_progress_ratio"}


def nodes(root: dict, path: str = "root"):
    yield path, root
    for child in root.get("children") or []:
        if isinstance(child, dict):
            yield from nodes(child, path + "/" + str(child.get("name", "?")))


def fail(path: str, message: str) -> None:
    raise ComponentError(f"{path}: {message}")


def fields(value: Any, allowed: set[str], required: set[str], path: str) -> dict:
    if not isinstance(value, dict):
        fail(path, "must be an object")
    if set(value) - allowed:
        fail(path, f"unknown fields: {sorted(set(value) - allowed)}")
    if required - set(value):
        fail(path, f"missing fields: {sorted(required - set(value))}")
    if "evidence" in value and (
        not isinstance(value["evidence"], str) or not value["evidence"].strip()
    ):
        fail(path, "evidence must be a non-empty explanation")
    return value


def finite(value: Any, path: str) -> float:
    if not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value):
        fail(path, "must be a finite number")
    return value


def pair(value: Any, path: str) -> tuple[float, float]:
    fields(value, {"x", "y"}, {"x", "y"}, path)
    return finite(value["x"], path + ".x"), finite(value["y"], path + ".y")


def target(owner: dict, relative: Any, path: str, *, direct: bool = False) -> dict:
    if not isinstance(relative, str) or not relative:
        fail(path, "target must be a non-empty relative element path")
    if relative == "." and not direct:
        return owner
    parts = relative.split("/")
    if any(part in {"", ".", ".."} or "\\" in part for part in parts) or (direct and len(parts) != 1):
        fail(path, "target must name " + ("a direct child" if direct else "this node or a descendant"))
    node = owner
    for part in parts:
        node = next((c for c in node.get("children", []) if c.get("name") == part), None)
        if node is None:
            fail(path, f"target does not exist: {relative}")
    return node


def validate_components(structure: dict) -> None:
    canvas = structure.get("canvas") or {}
    insets = fields(canvas.get("safeArea", {}), {"left", "top", "right", "bottom"}, set(), "canvas.safeArea")
    for side, value in insets.items():
        if finite(value, f"canvas.safeArea.{side}") < 0:
            fail("canvas.safeArea", "insets must be non-negative")
    for extent, start, end in (("width", "left", "right"), ("height", "top", "bottom")):
        if insets and insets.get(start, 0) + insets.get(end, 0) >= canvas.get(extent, 0):
            fail("canvas.safeArea", f"insets leave no usable {extent}")

    parents = {}
    for path, node in nodes(structure["root"]):
        for child in node.get("children") or []:
            parents[path + "/" + child["name"]] = node
        if "visible" in node and not isinstance(node["visible"], bool):
            fail(path, "visible must be a boolean")
        if "responsive" in node:
            spec = fields(node["responsive"], {"min", "max", "offsetMin", "offsetMax", "safeArea", "evidence"},
                          {"min", "max", "offsetMin", "offsetMax"}, path + ".responsive")
            if path == "root":
                fail(path, "root follows canvas size; responsive belongs on child nodes")
            if parents[path].get("layout") or any(k in node for k in ("align", "vAlign", "offset")):
                fail(path, "responsive cannot compete with layout, align, vAlign, or offset")
            low, high = pair(spec["min"], path + ".responsive.min"), pair(spec["max"], path + ".responsive.max")
            pair(spec["offsetMin"], path + ".responsive.offsetMin")
            pair(spec["offsetMax"], path + ".responsive.offsetMax")
            if any(not 0 <= a <= b <= 1 for a, b in zip(low, high)):
                fail(path, "responsive min/max must satisfy 0 <= min <= max <= 1")
            if "safeArea" in spec and not isinstance(spec["safeArea"], bool):
                fail(path, "responsive.safeArea must be a boolean")
            if spec.get("safeArea") and parents[path] is not structure["root"]:
                fail(path, "safeArea applies only to direct canvas-root children")
        if "state" in node:
            spec = fields(node["state"], {"current", "variants", "evidence"}, {"current", "variants"}, path + ".state")
            variants = spec["variants"]
            if not isinstance(variants, dict) or not variants or any(not isinstance(k, str) or not k for k in variants):
                fail(path, "state.variants must be a non-empty map of state names")
            if not isinstance(spec["current"], str) or spec["current"] not in variants:
                fail(path, "state.current must name a declared variant")
            for name, patches in variants.items():
                if not isinstance(patches, dict):
                    fail(path, f"state variant {name} must map relative paths to property overrides")
                for relative, patch in patches.items():
                    target(node, relative, path + ".state")
                    fields(patch, STATE_FIELDS, set(), path + f".state.{name}.{relative}")
                    for key, value in patch.items():
                        if key == "visible" and not isinstance(value, bool):
                            fail(path, "state visibility must be boolean")
                        if key in {"asset", "text", "fontFamily"} and (not isinstance(value, str) or not value.strip()):
                            fail(path, f"state {key} must be a non-empty string")
                        if key in {"opacity", "hueShift", "fontSize", "lineHeight", "textScaleX", "strokeWidth"}:
                            finite(value, path + f".state.{name}.{key}")
        if "progress" in node and "scroll" in node:
            fail(path, "progress and scroll must use separate component nodes")
        if "progress" in node:
            spec = fields(node["progress"], {"min", "max", "value", "fill", "direction", "label", "format", "evidence"},
                          {"value", "fill", "direction"}, path + ".progress")
            low, high, value = (finite(spec.get(k, default), path + ".progress." + k)
                                for k, default in (("min", 0), ("max", 1), ("value", 0)))
            if low >= high or not low <= value <= high:
                fail(path, "progress requires min < max and min <= value <= max")
            if not isinstance(spec["direction"], str) or spec["direction"] not in DIRECTIONS:
                fail(path, f"progress.direction must be one of {sorted(DIRECTIONS)}")
            fill = target(node, spec["fill"], path + ".progress.fill", direct=True)
            if not (fill.get("asset") or fill.get("color") or fill.get("children")):
                fail(path, "progress fill must have visible artwork or children")
            if "label" in spec:
                label = target(node, spec["label"], path + ".progress.label", direct=True)
                if label is fill or label.get("type") != "text":
                    fail(path, "progress label must reference a separate text child")
            elif "format" in spec:
                fail(path, "progress.format requires a label")
            template = spec.get("format", "{percent:.0f}%")
            if not isinstance(template, str) or not template.strip():
                fail(path, "progress.format must be a non-empty string")
            try:
                for _, field, _, conversion in string.Formatter().parse(template):
                    if field is not None and field not in {"value", "min", "max", "percent"}:
                        fail(path, "progress format accepts only value, min, max, percent")
                    if conversion:
                        fail(path, "progress format conversions are unsupported")
                template.format(value=value, min=low, max=high, percent=(value-low)/(high-low)*100)
            except (ValueError, KeyError, IndexError, TypeError) as exc:
                fail(path, f"invalid progress format: {exc}")
        if "scroll" in node:
            spec = fields(node["scroll"], {"content", "direction", "offset", "evidence"},
                          {"content", "direction"}, path + ".scroll")
            if node.get("type") != "container" or node.get("layout"):
                fail(path, "scroll viewport must be a container without its own layout")
            if not isinstance(spec["direction"], str) or spec["direction"] not in {"horizontal", "vertical", "both"}:
                fail(path, "scroll.direction must be horizontal, vertical, or both")
            content = target(node, spec["content"], path + ".scroll.content", direct=True)
            if content.get("type") != "container":
                fail(path, "scroll content must be a container")
            x, y = pair(spec.get("offset", {"x": 0, "y": 0}), path + ".scroll.offset")
            if x < 0 or y < 0:
                fail(path, "scroll offsets must be non-negative")
            if (spec["direction"] == "vertical" and x != 0) or (spec["direction"] == "horizontal" and y != 0):
                fail(path, "scroll offset uses a disabled axis")


def state_patch_nodes(structure: dict):
    """Yield merged nodes so the main validator checks even inactive assets."""
    parents = {}
    for path, node in nodes(structure["root"]):
        for child in node.get("children") or []:
            parents[path + "/" + child["name"]] = node
    for path, owner in nodes(structure["root"]):
        for name, patches in owner.get("state", {}).get("variants", {}).items():
            for relative, patch in patches.items():
                merged = copy.deepcopy(target(owner, relative, path))
                merged.update(copy.deepcopy(patch))
                full_path = path if relative == "." else path + "/" + relative
                parent = parents.get(full_path, {})
                yield (f"{path}@{name}/{relative}", merged,
                       parent.get("layout", {}).get("type"), parent.get("role"))


def snapshot(structure: dict, *, size=None, states=None, progress=None, scroll=None) -> dict:
    """Build an independent scenario; no authored node or baseline is modified."""
    scene = copy.deepcopy(structure)
    for _, node in nodes(scene["root"]):
        for key in ("_abs", "_rel", *RUNTIME_FIELDS):
            node.pop(key, None)
    by_path = dict(nodes(scene["root"]))
    for values, field in ((states, "state"), (progress, "progress"), (scroll, "scroll")):
        for path, value in (values or {}).items():
            if path not in by_path or field not in by_path[path]:
                fail(path, f"no {field} component at this path")
            key = {"state": "current", "progress": "value", "scroll": "offset"}[field]
            by_path[path][field][key] = copy.deepcopy(value)
    if size is not None:
        if len(size) != 2 or any(finite(v, "preview.size") <= 0 or int(v) != v for v in size):
            fail("preview", "size must contain two positive integers")
        scene["canvas"].update(width=int(size[0]), height=int(size[1]))
        scene["root"]["size"] = {"width": int(size[0]), "height": int(size[1])}
    validate_components(scene)
    for path, node in nodes(scene["root"]):
        state = node.get("state")
        if state:
            for relative, patch in state["variants"][state["current"]].items():
                target(node, relative, path).update(copy.deepcopy(patch))
    for path, node in nodes(scene["root"]):
        spec = node.get("progress")
        if spec and "label" in spec:
            low, high = spec.get("min", 0), spec.get("max", 1)
            target(node, spec["label"], path)["text"] = spec.get("format", "{percent:.0f}%").format(
                value=spec["value"], min=low, max=high, percent=(spec["value"]-low)/(high-low)*100,
            )
    return scene


def responsive_box(spec: dict, width: int, height: int, safe_area: dict) -> tuple[int, int, int, int]:
    left = safe_area.get("left", 0) if spec.get("safeArea") else 0
    top = safe_area.get("top", 0) if spec.get("safeArea") else 0
    width -= left + (safe_area.get("right", 0) if spec.get("safeArea") else 0)
    height -= top + (safe_area.get("bottom", 0) if spec.get("safeArea") else 0)
    x = round(left + width * spec["min"]["x"] + spec["offsetMin"]["x"])
    y = round(top + height * spec["min"]["y"] + spec["offsetMin"]["y"])
    right = round(left + width * spec["max"]["x"] + spec["offsetMax"]["x"])
    bottom = round(top + height * spec["max"]["y"] + spec["offsetMax"]["y"])
    if right <= x or bottom <= y:
        raise ComponentError("responsive constraints produce a non-positive size")
    return x, y, right-x, bottom-y


def intersection(a, b):
    if a is None or b is None:
        return None
    x, y = max(a[0], b[0]), max(a[1], b[1])
    right, bottom = min(a[0]+a[2], b[0]+b[2]), min(a[1]+a[3], b[1]+b[3])
    return [x, y, max(0, right-x), max(0, bottom-y)]


def apply_progress_clips(structure: dict) -> None:
    for path, node in nodes(structure["root"]):
        spec = node.get("progress")
        if not spec:
            continue
        low, high = spec.get("min", 0), spec.get("max", 1)
        ratio = (spec["value"] - low) / (high - low)
        node["_progress_ratio"] = ratio
        fill = target(node, spec["fill"], path, direct=True)
        x, y, width, height = fill["_abs"]
        direction = spec["direction"]
        if direction in {"left-to-right", "right-to-left"}:
            extent = round(width * ratio)
            x += width - extent if direction == "right-to-left" else 0
            width = extent
        else:
            extent = round(height * ratio)
            y += height - extent if direction == "bottom-to-top" else 0
            height = extent
        clip = [x, y, width, height]
        fill["_clip_bbox"] = intersection(fill["_clip_bbox"], clip) if "_clip_bbox" in fill else clip
