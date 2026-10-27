"""
Render a side-by-side comparison between a UI design image and the reconstruction
produced from ui_structure.json + sliced assets.

Usage:
    py -B render_comparison.py \
        --design path/to/design.png \
        --structure path/to/ui_structure.json \
        --assets path/to/sprite_dir \
        --output path/to/comparison.png

The left panel shows the original design, the right panel shows the reconstruction.
Both are scaled to the same height for visual comparison.
"""

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont, __version__ as PILLOW_VERSION

# Make sibling layout.py importable when run from anywhere
sys.path.insert(0, str(Path(__file__).resolve().parent))
import ui_components as components
import layout as layout_mod  # noqa: E402
from annotate_grid import draw_grid, select_grid_palette  # noqa: E402


class RenderError(RuntimeError):
    """Raised when an asset cannot be rendered at its requested size."""


# --- Asset loading ---------------------------------------------------------

class AssetCache:
    """Loads images from the assets directory tree on demand."""

    def __init__(self, assets_dir: Path):
        self.assets_dir = assets_dir
        # Build case-insensitive indexes so "Foo.Png" matches "foo.png".
        # Basenames are accepted when unique; relative paths disambiguate
        # duplicate filenames in nested asset folders.
        self._index = {}
        self._borders = {}
        self._file_count = 0
        self._border_file_count = 0
        duplicates = set()
        for p in assets_dir.rglob("*"):
            if p.is_file() and p.suffix.lower() in (".png", ".jpg", ".jpeg"):
                self._file_count += 1
                rel_key = p.relative_to(assets_dir).as_posix().lower()
                name_key = p.name.lower()
                self._index[rel_key] = p
                if name_key in self._index and self._index[name_key] != p:
                    duplicates.add(name_key)
                else:
                    self._index[name_key] = p
                border = self._read_unity_sprite_border(p)
                if border:
                    self._border_file_count += 1
                    self._borders[rel_key] = border
                    if name_key not in duplicates:
                        self._borders[name_key] = border
        for name_key in duplicates:
            self._index.pop(name_key, None)
            self._borders.pop(name_key, None)
        self._cache = {}

    def path_for(self, filename: str):
        if not filename:
            return None
        return self._index.get(filename.replace("\\", "/").lower())

    @staticmethod
    def _read_unity_sprite_border(path: Path):
        """Read Unity spriteBorder from the sidecar .meta file, if present.

        Unity stores Sprite.border as x=left, y=bottom, z=right, w=top.
        The renderer wants left/top/right/bottom.
        """
        meta_path = Path(str(path) + ".meta")
        if not meta_path.exists():
            return None
        try:
            text = meta_path.read_text(encoding="utf-8")
        except OSError:
            return None
        match = re.search(
            r"spriteBorder:\s*\{x:\s*([^,]+),\s*y:\s*([^,]+),\s*z:\s*([^,]+),\s*w:\s*([^}]+)\}",
            text,
        )
        if not match:
            return None
        left, bottom, right, top = (int(float(v.strip())) for v in match.groups())
        if left == top == right == bottom == 0:
            return None
        return {"left": left, "top": top, "right": right, "bottom": bottom}

    def get(self, filename: str):
        if not filename:
            return None
        key = filename.replace("\\", "/").lower()
        if key in self._cache:
            return self._cache[key]
        path = self._index.get(key)
        if path is None:
            print(f"  [warn] asset not found: {filename}", file=sys.stderr)
            self._cache[key] = None
            return None
        img = Image.open(path).convert("RGBA")
        self._cache[key] = img
        return img

    def border_for(self, filename: str):
        if not filename:
            return None
        return self._borders.get(filename.replace("\\", "/").lower())


# --- Rendering helpers -----------------------------------------------------

def hex_to_rgba(hex_color: str, alpha: float = 1.0):
    if not hex_color:
        return None
    s = hex_color.lstrip("#")
    if len(s) == 6:
        r, g, b = int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)
        a = int(255 * alpha)
    elif len(s) == 8:
        r, g, b, a = (int(s[i:i + 2], 16) for i in (0, 2, 4, 6))
        a = int(a * alpha)
    else:
        return None
    return (r, g, b, a)


def apply_tint(img: Image.Image, hex_color: str) -> Image.Image:
    """Multiply-tint an RGBA image by a hex color (preserves alpha)."""
    if not hex_color:
        return img
    tint = hex_to_rgba(hex_color)
    if tint is None:
        return img
    tr, tg, tb, _ = tint
    arr = np.array(img, dtype=np.float32)
    arr[..., 0] *= tr / 255.0
    arr[..., 1] *= tg / 255.0
    arr[..., 2] *= tb / 255.0
    arr = np.clip(arr, 0, 255).astype(np.uint8)
    return Image.fromarray(arr, mode="RGBA")


def apply_hue_shift(img: Image.Image, degrees: int | float) -> Image.Image:
    """Rotate visible RGB hues while preserving alpha and hidden pixels."""
    if (
        not isinstance(degrees, (int, float))
        or isinstance(degrees, bool)
        or not math.isfinite(degrees)
        or degrees < -360
        or degrees > 360
    ):
        raise ValueError("hueShift must be a finite number between -360 and 360")
    if degrees == 0:
        return img

    rgba = np.asarray(img.convert("RGBA"), dtype=np.uint8)
    hsv = np.asarray(
        Image.fromarray(rgba[..., :3], mode="RGB").convert("HSV"),
        dtype=np.uint8,
    ).copy()
    delta = int(round(float(degrees) * 256.0 / 360.0))
    hsv[..., 0] = (
        hsv[..., 0].astype(np.int16) + delta
    ) % 256
    shifted_rgb = np.asarray(
        Image.fromarray(hsv, mode="HSV").convert("RGB"),
        dtype=np.uint8,
    )
    visible = rgba[..., 3:4] > 0
    result = np.concatenate(
        [np.where(visible, shifted_rgb, rgba[..., :3]), rgba[..., 3:4]],
        axis=2,
    )
    return Image.fromarray(result.astype(np.uint8), mode="RGBA")


def effective_nine_slice_margins(
    margins: dict,
    src_w: int,
    src_h: int,
    target_w: int,
    target_h: int,
) -> dict[str, int]:
    """Clamp nine-slice margins to the source and target pixel extents."""
    l = int(margins.get("left", 0))
    t = int(margins.get("top", 0))
    r = int(margins.get("right", 0))
    b = int(margins.get("bottom", 0))

    def clamp_pair(first: int, second: int, source_extent: int,
                   target_extent: int) -> tuple[int, int]:
        # Some Unity sprites intentionally use borders whose pair sums to the
        # full source dimension. PIL crop/resize needs a non-empty stretch
        # strip, so keep at least one source and target pixel for the center.
        source_limit = max(0, source_extent - 1)
        target_limit = max(0, target_extent - 1)
        first = max(0, min(first, source_limit, target_limit))
        second = max(0, min(second, source_limit, target_limit))
        for limit in (source_limit, target_limit):
            total = first + second
            if limit > 0 and total > limit:
                first = int(round(first * limit / total))
                second = limit - first
        return first, second

    l, r = clamp_pair(l, r, src_w, target_w)
    t, b = clamp_pair(t, b, src_h, target_h)
    return {"left": l, "top": t, "right": r, "bottom": b}


def render_nine_slice(src: Image.Image, target_w: int, target_h: int,
                      margins: dict) -> Image.Image:
    """Render `src` at (target_w, target_h) using 9-slice scaling.

    margins: dict with integer keys "left", "top", "right", "bottom" giving
    the non-stretching corner widths/heights in source-pixel units. The four
    corners are pasted unchanged, the four edges stretch along one axis, and
    the center stretches on both axes.
    """
    sw, sh = src.size
    effective = effective_nine_slice_margins(
        margins, sw, sh, target_w, target_h
    )
    l = effective["left"]
    t = effective["top"]
    r = effective["right"]
    b = effective["bottom"]

    out = Image.new("RGBA", (max(1, target_w), max(1, target_h)), (0, 0, 0, 0))

    # Source crops: corners, edges, center
    tl = src.crop((0, 0, l, t))
    tr = src.crop((sw - r, 0, sw, t))
    bl = src.crop((0, sh - b, l, sh))
    br = src.crop((sw - r, sh - b, sw, sh))

    top_edge = src.crop((l, 0, sw - r, t))
    bot_edge = src.crop((l, sh - b, sw - r, sh))
    left_edge = src.crop((0, t, l, sh - b))
    right_edge = src.crop((sw - r, t, sw, sh - b))

    center = src.crop((l, t, sw - r, sh - b))

    # Stretch sizes in target
    cx = target_w - l - r
    cy = target_h - t - b

    # Corners (no stretch)
    if l > 0 and t > 0:
        out.paste(tl, (0, 0))
    if r > 0 and t > 0:
        out.paste(tr, (target_w - r, 0))
    if l > 0 and b > 0:
        out.paste(bl, (0, target_h - b))
    if r > 0 and b > 0:
        out.paste(br, (target_w - r, target_h - b))

    # Edges (stretch one axis)
    if cx > 0 and t > 0 and top_edge.width > 0:
        out.paste(top_edge.resize((cx, t), Image.LANCZOS), (l, 0))
    if cx > 0 and b > 0 and bot_edge.width > 0:
        out.paste(bot_edge.resize((cx, b), Image.LANCZOS), (l, target_h - b))
    if cy > 0 and l > 0 and left_edge.height > 0:
        out.paste(left_edge.resize((l, cy), Image.LANCZOS), (0, t))
    if cy > 0 and r > 0 and right_edge.height > 0:
        out.paste(right_edge.resize((r, cy), Image.LANCZOS), (target_w - r, t))

    # Center (stretch both)
    if cx > 0 and cy > 0 and center.width > 0 and center.height > 0:
        out.paste(center.resize((cx, cy), Image.LANCZOS), (l, t))

    return out


def resolve_nine_slice_margins(nine_slice, src_w: int, src_h: int,
                               meta_border: dict | None = None) -> dict:
    """Convert `nineSlice` JSON value into explicit margins dict.

    Accepts:
      - True / "auto" / "meta": use Unity .meta spriteBorder if present;
                     otherwise use min(sw, sh) // 4, capped at 60 px
      - int N:       N pixels on all 4 sides
      - dict:        {"left": ..., "top": ..., "right": ..., "bottom": ...}
                     Missing keys default to 0 (useful for things like a bar
                     that only stretches horizontally: {"left": 20, "right": 20})
    """
    if nine_slice is True or (
        isinstance(nine_slice, str) and nine_slice in ("auto", "meta")
    ):
        if meta_border:
            return {
                "left": int(meta_border.get("left", 0)),
                "top": int(meta_border.get("top", 0)),
                "right": int(meta_border.get("right", 0)),
                "bottom": int(meta_border.get("bottom", 0)),
            }
        m = max(4, min(60, min(src_w, src_h) // 4))
        return {"left": m, "top": m, "right": m, "bottom": m}
    if isinstance(nine_slice, (int, float)) and not isinstance(nine_slice, bool):
        m = int(nine_slice)
        return {"left": m, "top": m, "right": m, "bottom": m}
    if isinstance(nine_slice, dict):
        return {
            "left": int(nine_slice.get("left", 0)),
            "top": int(nine_slice.get("top", 0)),
            "right": int(nine_slice.get("right", 0)),
            "bottom": int(nine_slice.get("bottom", 0)),
        }
    return {"left": 0, "top": 0, "right": 0, "bottom": 0}


def apply_opacity(img: Image.Image, opacity: float) -> Image.Image:
    if opacity is None or opacity >= 1.0:
        return img
    arr = np.array(img, dtype=np.float32)
    arr[..., 3] *= max(0.0, min(1.0, opacity))
    arr = np.clip(arr, 0, 255).astype(np.uint8)
    return Image.fromarray(arr, mode="RGBA")


FONT_SEARCH_DIRS: list[Path] = []

SYSTEM_FONT_CANDIDATES = (
    Path("C:/Windows/Fonts/arialbd.ttf"),
    Path("C:/Windows/Fonts/arial.ttf"),
    Path("C:/Windows/Fonts/segoeui.ttf"),
    Path("C:/Windows/Fonts/msyh.ttc"),
    Path("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"),
    Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
    Path("/System/Library/Fonts/Helvetica.ttc"),
)


def preferred_font_candidates(preferred: str) -> list[Path]:
    """Return deterministic task paths for an explicit font request."""
    requested = Path(preferred)
    if requested.is_absolute():
        return [requested]

    candidates: list[Path] = []
    for configured_root in FONT_SEARCH_DIRS:
        root = Path(configured_root)
        if not root.is_absolute():
            continue
        candidate = root / requested
        candidates.append(candidate)
        if requested.suffix:
            continue

        parent = candidate.parent
        stem = candidate.name
        matches = [
            *parent.glob(f"{stem}*.ttf"),
            *parent.glob(f"{stem}*.otf"),
        ]
        candidates.extend(sorted(matches, key=lambda path: path.name.casefold()))
    return candidates


def find_font(size: int, preferred: str | None = None) -> ImageFont.FreeTypeFont:
    # Try a few common fonts; fall back to default bitmap font
    candidates = preferred_font_candidates(preferred) if preferred else []
    for candidate in [*candidates, *SYSTEM_FONT_CANDIDATES]:
        if candidate.is_file():
            try:
                return ImageFont.truetype(str(candidate), size)
            except Exception:
                continue
    return ImageFont.load_default()


def font_path_matches_request(resolved: str | None, preferred: str) -> bool:
    if not resolved:
        return False
    requested = Path(preferred)
    if not (
        requested.is_absolute()
        or requested.suffix
        or requested.parent != Path(".")
    ):
        return False

    resolved_path = Path(resolved).resolve()
    return any(
        resolved_path == candidate.resolve()
        for candidate in preferred_font_candidates(preferred)
    )


def describe_font(font: ImageFont.ImageFont, preferred: str | None) -> dict:
    """Return conservative font-resolution details for render diagnostics."""
    resolved = getattr(font, "path", None)
    if isinstance(resolved, bytes):
        resolved = resolved.decode(errors="replace")
    try:
        resolved_path = Path(resolved)
        resolved_text = str(resolved_path) if resolved_path.is_file() else None
    except (OSError, TypeError, ValueError):
        # Pillow's embedded default font is commonly backed by BytesIO. Its
        # repr contains a process-specific address and is not a dependency
        # file that the workflow can snapshot.
        resolved_text = None

    resolved_sha256 = None
    if resolved_text:
        try:
            digest = hashlib.sha256()
            with Path(resolved_text).open("rb") as handle:
                for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                    digest.update(chunk)
            resolved_sha256 = digest.hexdigest()
        except OSError:
            pass

    fallback = False
    if preferred:
        requested_path = Path(preferred)
        if (
            requested_path.is_absolute()
            or requested_path.suffix
            or requested_path.parent != Path(".")
        ):
            fallback = not font_path_matches_request(resolved_text, preferred)
        else:
            requested_stem = re.sub(
                r"[^a-z0-9]", "", requested_path.stem.lower()
            )
            resolved_names = []
            if resolved_text:
                resolved_names.append(Path(resolved_text).stem)
            try:
                family, style = font.getname()
                resolved_names.extend((family, f"{family}{style}"))
            except (AttributeError, OSError):
                pass
            normalized_names = [
                re.sub(r"[^a-z0-9]", "", name.lower())
                for name in resolved_names
            ]
            fallback = not (
                requested_stem
                and any(
                    name and (requested_stem in name or name in requested_stem)
                    for name in normalized_names
                )
            )
    return {
        "requested": preferred,
        "resolved": resolved_text,
        "resolved_sha256": resolved_sha256,
        "pillow_version": PILLOW_VERSION,
        "fallback": fallback,
    }


def overflow_sides(inner: list[int] | None, outer: list[int]) -> dict[str, int]:
    if inner is None:
        return {"left": 0, "top": 0, "right": 0, "bottom": 0}
    ix, iy, iw, ih = inner
    ox, oy, ow, oh = outer
    return {
        "left": max(0, ox - ix),
        "top": max(0, oy - iy),
        "right": max(0, ix + iw - (ox + ow)),
        "bottom": max(0, iy + ih - (oy + oh)),
    }


def union_bboxes(boxes: list[list[int] | None]) -> list[int] | None:
    visible = [box for box in boxes if box is not None]
    if not visible:
        return None
    left = min(box[0] for box in visible)
    top = min(box[1] for box in visible)
    right = max(box[0] + box[2] for box in visible)
    bottom = max(box[1] + box[3] for box in visible)
    return [left, top, max(0, right - left), max(0, bottom - top)]


def clip_bbox(bbox: list[int] | None,
              canvas_size: tuple[int, int]) -> list[int] | None:
    if bbox is None:
        return None
    x, y, width, height = bbox
    canvas_width, canvas_height = canvas_size
    left = max(0, x)
    top = max(0, y)
    right = min(canvas_width, x + width)
    bottom = min(canvas_height, y + height)
    if right <= left or bottom <= top:
        return None
    return [left, top, right - left, bottom - top]


def alpha_layer_metrics(
    image: Image.Image,
    x: int,
    y: int,
    canvas_size: tuple[int, int],
) -> tuple[list[int] | None, bool]:
    """Return visible alpha bounds and conservative opaque-layer evidence."""
    canvas_width, canvas_height = canvas_size
    draw_left = max(0, x)
    draw_top = max(0, y)
    draw_right = min(canvas_width, x + image.width)
    draw_bottom = min(canvas_height, y + image.height)
    if draw_right <= draw_left or draw_bottom <= draw_top:
        return None, False

    visible_alpha = image.getchannel("A").crop((
        draw_left - x,
        draw_top - y,
        draw_right - x,
        draw_bottom - y,
    ))
    alpha_bbox = visible_alpha.getbbox()
    if alpha_bbox is None:
        return None, False

    left, top, right, bottom = alpha_bbox
    visible_bbox = [
        draw_left + left,
        draw_top + top,
        right - left,
        bottom - top,
    ]
    return visible_bbox, visible_alpha.getextrema() == (255, 255)


def alpha_visible_bbox(image: Image.Image, x: int, y: int,
                       canvas_size: tuple[int, int]) -> list[int] | None:
    return alpha_layer_metrics(image, x, y, canvas_size)[0]


def rasterize_text_line(
    line: str,
    bbox: tuple[int, int, int, int],
    font: ImageFont.ImageFont,
    fill,
    stroke_width: int,
    stroke_fill,
) -> Image.Image:
    width = max(1, bbox[2] - bbox[0])
    height = max(1, bbox[3] - bbox[1])
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    ImageDraw.Draw(image).text(
        (-bbox[0], -bbox[1]),
        line,
        font=font,
        fill=fill,
        stroke_width=stroke_width,
        stroke_fill=stroke_fill,
    )
    return image


def draw_text(canvas: Image.Image, elem: dict, origin_x: int, origin_y: int):
    text = elem.get("text", "")
    if not text:
        return None
    pos = elem.get("position", {"x": 0, "y": 0})
    size = elem.get("size", {"width": 100, "height": 40})
    x = origin_x + int(pos.get("x", 0))
    y = origin_y + int(pos.get("y", 0))
    w = int(size.get("width", 100))
    h = int(size.get("height", 40))

    font_size = int(elem.get("fontSize", 20))
    color_rgba = hex_to_rgba(elem.get("color", "#000000"), elem.get("opacity", 1.0))
    alignment = elem.get("alignment", "left")
    text_v_align = elem.get("textVAlign", "middle")
    line_h = max(1, int(elem.get("lineHeight", font_size + 4)))
    stroke_width = int(elem.get("strokeWidth", 0))
    stroke_rgba = hex_to_rgba(elem.get("strokeColor", "#000000"), elem.get("opacity", 1.0))
    raw_text_scale_x = elem.get("textScaleX", 1.0)
    if (
        not isinstance(raw_text_scale_x, (int, float))
        or isinstance(raw_text_scale_x, bool)
        or raw_text_scale_x <= 0
    ):
        raise ValueError("textScaleX must be a positive finite number")
    try:
        text_scale_x = float(raw_text_scale_x)
    except OverflowError as exc:
        raise ValueError("textScaleX must be a positive finite number") from exc
    if not math.isfinite(text_scale_x):
        raise ValueError("textScaleX must be a positive finite number")

    preferred_font = elem.get("fontFamily") or elem.get("font")
    font = find_font(font_size, preferred_font)
    draw = ImageDraw.Draw(canvas)

    lines = text.split("\n")
    line_bboxes = []
    line_rasters = []
    for line in lines:
        if not line:
            bbox = (0, 0, 0, 0)
        else:
            try:
                bbox = draw.textbbox(
                    (0, 0),
                    line,
                    font=font,
                    stroke_width=stroke_width,
                )
            except Exception:
                try:
                    raw = font.getbbox(line)
                    bbox = (
                        raw[0] - stroke_width,
                        raw[1] - stroke_width,
                        raw[2] + stroke_width,
                        raw[3] + stroke_width,
                    )
                except Exception:
                    width = font_size * len(line) // 2
                    bbox = (-stroke_width, -stroke_width,
                            width + stroke_width, font_size + stroke_width)
        line_image = rasterize_text_line(
            line,
            bbox,
            font,
            color_rgba,
            stroke_width,
            stroke_rgba,
        )
        line_bboxes.append(bbox)
        line_rasters.append((line_image, line_image.getchannel("A").getbbox()))

    vertical_bounds = [
        (
            i * line_h + bbox[1] + visible[1],
            i * line_h + bbox[1] + visible[3],
        )
        for i, (bbox, (_image, visible)) in enumerate(
            zip(line_bboxes, line_rasters)
        )
        if visible is not None
    ]
    if vertical_bounds:
        block_top = min(top for top, _bottom in vertical_bounds)
        block_bottom = max(bottom for _top, bottom in vertical_bounds)
    else:
        block_top = 0
        block_bottom = 0
    block_h = max(0, block_bottom - block_top)
    if text_v_align in ("top", "start"):
        ink_top = y
    elif text_v_align in ("bottom", "end"):
        ink_top = y + h - block_h
    else:
        ink_top = y + (h - block_h) // 2
    draw_origin_y = ink_top - block_top

    line_ink_bboxes: list[list[int] | None] = []
    for i, (line, bbox, (line_image, local_visible)) in enumerate(
        zip(lines, line_bboxes, line_rasters)
    ):
        if local_visible is None:
            line_ink_bboxes.append(None)
            continue

        if text_scale_x == 1.0:
            visible_width = local_visible[2] - local_visible[0]
            visible_height = local_visible[3] - local_visible[1]
            if alignment == "center":
                line_left = x + (w - visible_width) // 2
            elif alignment == "right":
                line_left = x + w - visible_width
            else:
                line_left = x
            line_top = (
                draw_origin_y + i * line_h + bbox[1] + local_visible[1]
            )
            draw.text(
                (line_left - bbox[0] - local_visible[0],
                 draw_origin_y + i * line_h),
                line,
                font=font,
                fill=color_rgba,
                stroke_width=stroke_width,
                stroke_fill=stroke_rgba,
            )
            line_ink_bboxes.append([
                line_left,
                line_top,
                visible_width,
                visible_height,
            ])
            continue

        visible_line = line_image.crop(local_visible)
        scaled_width = max(1, int(round(visible_line.width * text_scale_x)))
        if scaled_width != visible_line.width:
            visible_line = visible_line.resize(
                (scaled_width, visible_line.height), Image.LANCZOS
            )
        scaled_visible = visible_line.getchannel("A").getbbox()
        if scaled_visible is None:
            line_ink_bboxes.append(None)
            continue
        scaled_local_left, scaled_local_top, scaled_right, scaled_bottom = (
            scaled_visible
        )
        visible_line = visible_line.crop(scaled_visible)

        if alignment == "center":
            line_left = x + (w - visible_line.width) // 2
        elif alignment == "right":
            line_left = x + w - visible_line.width
        else:
            line_left = x
        line_top = (
            draw_origin_y
            + i * line_h
            + bbox[1]
            + local_visible[1]
            + scaled_local_top
        )
        canvas.alpha_composite(visible_line, (line_left, line_top))
        line_ink_bboxes.append([
            line_left,
            line_top,
            scaled_right - scaled_local_left,
            scaled_bottom - scaled_local_top,
        ])

    ink_bbox = union_bboxes(line_ink_bboxes)
    visible_bbox = union_bboxes([
        clip_bbox(box, canvas.size) for box in line_ink_bboxes
    ])
    text_box = [x, y, w, h]
    overflow = overflow_sides(ink_bbox, text_box)
    return {
        "text_box": text_box,
        "ink_bbox": ink_bbox,
        "line_ink_bboxes": line_ink_bboxes,
        "visible_bbox": visible_bbox,
        "ink_overflow": overflow,
        "font": describe_font(font, preferred_font),
        "font_size": font_size,
        "line_height": line_h,
        "text_scale_x": text_scale_x,
        "stroke_width": stroke_width,
        "alignment": alignment,
        "vertical_alignment": text_v_align,
        "line_count": len(lines),
    }


def append_trace_entry(trace: list[dict] | None, entry: dict) -> None:
    if trace is None:
        return
    entry["z_order"] = len(trace)
    trace.append(entry)


def render_element(canvas: Image.Image, elem: dict, origin_x: int, origin_y: int,
                   assets: AssetCache, trace: list[dict] | None = None,
                   path: str = "root", parent_bbox: list[int] | None = None,
                   clip_bbox: list[int] | None = None):
    """Recursively render an element and its children onto the canvas.

    Position source priority:
      1. `_rel` annotated by layout.resolve_positions (preferred; honors
         layout/align/offset)
      2. `position` field (legacy / unresolved)
    """
    output_canvas = canvas
    own_clip = elem.get("_clip_bbox")
    if own_clip is not None:
        clip_bbox = own_clip if clip_bbox is None else components.intersection(clip_bbox, own_clip)
    if clip_bbox is not None:
        canvas = Image.new("RGBA", output_canvas.size, (0, 0, 0, 0))

    etype = elem.get("type", "container")
    rel = elem.get("_rel")
    if rel is not None:
        rx, ry, w, h = int(rel[0]), int(rel[1]), int(rel[2]), int(rel[3])
    else:
        pos = elem.get("position", {"x": 0, "y": 0})
        size = elem.get("size", {"width": 0, "height": 0})
        rx, ry = int(pos.get("x", 0)), int(pos.get("y", 0))
        w, h = int(size.get("width", 0)), int(size.get("height", 0))

    ax = origin_x + rx
    ay = origin_y + ry

    # Draw solid color rectangle if there's a color but no asset
    asset_name = elem.get("asset")
    color = elem.get("color")
    opacity = float(elem.get("opacity", 1.0))
    entry = {
        "path": path,
        "name": elem.get("name", "?"),
        "type": etype,
        "bbox": [ax, ay, w, h],
        "parent_bbox": parent_bbox,
        "opacity": opacity,
        "fully_opaque": False,
    }
    if asset_name or (color and etype != "text") or etype == "text":
        entry["visible_bbox"] = None
    if elem.get("_scroll_content"):
        entry["scroll_content"] = True
    if "_scroll_metrics" in elem:
        entry["scroll"] = elem["_scroll_metrics"]
    if "_progress_ratio" in elem:
        entry["progress_ratio"] = elem["_progress_ratio"]
    if "state" in elem:
        entry["state"] = elem["state"]["current"]
    if elem.get("visible", True) is False:
        entry.update(visible_bbox=None, hidden=True)
        append_trace_entry(trace, entry)
        return
    if elem.get("layout"):
        entry["layout"] = elem["layout"]

    if asset_name:
        img = assets.get(asset_name)
        asset_path = assets.path_for(asset_name)
        entry["asset"] = {
            "requested": asset_name,
            "resolved": (
                asset_path.relative_to(assets.assets_dir).as_posix()
                if asset_path else None
            ),
            "found": img is not None,
            "render_error": None,
        }
        if img is not None:
            nine_slice = elem.get("nineSlice")
            hue_shift = elem.get("hueShift", 0)
            entry["asset"].update({
                "source_size": [img.width, img.height],
                "source_alpha_bbox": list(img.getchannel("A").getbbox() or (0, 0, 0, 0)),
                "render_mode": "nine_slice" if nine_slice else "stretch",
                "hue_shift": hue_shift,
            })
            if img.width > 0 and img.height > 0 and w > 0 and h > 0:
                scale_x = w / img.width
                scale_y = h / img.height
                entry["asset"]["aspect_scale_error"] = round(
                    max(scale_x / scale_y, scale_y / scale_x) - 1.0, 6
                )
            try:
                if nine_slice:
                    requested_margins = resolve_nine_slice_margins(
                        nine_slice,
                        img.width,
                        img.height,
                        assets.border_for(asset_name),
                    )
                    margins = effective_nine_slice_margins(
                        requested_margins,
                        img.width,
                        img.height,
                        max(1, w),
                        max(1, h),
                    )
                    entry["asset"]["nine_slice_margins"] = margins
                    img_resized = render_nine_slice(img, max(1, w), max(1, h), margins)
                else:
                    img_resized = img.resize((max(1, w), max(1, h)), Image.LANCZOS)
                img_resized = apply_hue_shift(img_resized, hue_shift)
                if color:
                    img_resized = apply_tint(img_resized, color)
                if opacity < 1.0:
                    img_resized = apply_opacity(img_resized, opacity)
                visible_bbox, fully_opaque = alpha_layer_metrics(
                    img_resized, ax, ay, canvas.size
                )
                entry["visible_bbox"] = visible_bbox
                entry["fully_opaque"] = fully_opaque
                canvas.alpha_composite(img_resized, (ax, ay))
            except Exception as exc:
                render_error = f"{type(exc).__name__}: {exc}"
                entry["asset"]["render_error"] = render_error
                append_trace_entry(trace, entry)
                render_kind = "nine-slice" if nine_slice else "stretch"
                raise RenderError(
                    f"{render_kind} render failed for asset {asset_name!r} "
                    f"at {path}: {render_error}"
                ) from exc
    elif color and etype != "text" and w > 0 and h > 0:
        fill = hex_to_rgba(color, opacity)
        if fill:
            overlay = Image.new("RGBA", (w, h), fill)
            visible_bbox, fully_opaque = alpha_layer_metrics(
                overlay, ax, ay, canvas.size
            )
            entry["visible_bbox"] = visible_bbox
            entry["fully_opaque"] = fully_opaque
            canvas.alpha_composite(overlay, (ax, ay))

    if etype == "text":
        # draw_text reads element.position; rebuild a temp elem with the
        # resolved relative position so legacy code still works.
        temp = dict(elem)
        temp["position"] = {"x": rx, "y": ry}
        temp["size"] = {"width": w, "height": h}
        entry["text"] = draw_text(canvas, temp, origin_x, origin_y)
        if entry["text"] is not None:
            entry["visible_bbox"] = entry["text"]["visible_bbox"]

    if clip_bbox is not None:
        entry["clip_bbox"] = list(clip_bbox)
        clipped = components.intersection(clip_bbox, [0, 0, *output_canvas.size])
        left, top, width, height = clipped
        if width > 0 and height > 0:
            layer = canvas.crop((left, top, left+width, top+height))
            visible_bbox, fully_opaque = alpha_layer_metrics(layer, left, top, output_canvas.size)
            output_canvas.alpha_composite(layer, (left, top))
        else:
            visible_bbox, fully_opaque = None, False
        if "visible_bbox" in entry:
            entry["visible_bbox"] = visible_bbox
            entry["fully_opaque"] = fully_opaque
        if entry.get("text") is not None:
            entry["text"]["visible_bbox"] = visible_bbox
        canvas = output_canvas
    append_trace_entry(trace, entry)

    # Recurse into children
    for child in elem.get("children", []) or []:
        child_name = child.get("name", "?") if isinstance(child, dict) else "?"
        render_element(
            canvas,
            child,
            ax,
            ay,
            assets,
            trace,
            f"{path}/{child_name}",
            [ax, ay, w, h],
            clip_bbox,
        )


# --- Comparison ------------------------------------------------------------

def build_side_by_side(design_img: Image.Image, reconstruction: Image.Image,
                       gap: int = 20, label_h: int = 40) -> Image.Image:
    """Stack design (left) and reconstruction (right) at the same height."""
    target_h = max(design_img.height, reconstruction.height)

    def scale_to_h(im, h):
        ratio = h / im.height
        new_w = max(1, int(im.width * ratio))
        return im.resize((new_w, h), Image.LANCZOS)

    left = scale_to_h(design_img.convert("RGBA"), target_h)
    right = scale_to_h(reconstruction, target_h)

    total_w = left.width + right.width + gap
    total_h = target_h + label_h
    combined = Image.new("RGBA", (total_w, total_h), (240, 240, 240, 255))
    combined.paste(left, (0, label_h), left)
    combined.paste(right, (left.width + gap, label_h), right)

    draw = ImageDraw.Draw(combined)
    font = find_font(22)
    draw.text((left.width // 2 - 40, 8), "Design", font=font, fill=(30, 30, 30, 255))
    draw.text((left.width + gap + right.width // 2 - 90, 8),
              "Reconstruction", font=font, fill=(30, 30, 30, 255))

    return combined


def render_from_structure(structure: dict, assets: AssetCache,
                          background: tuple[int, int, int, int],
                          trace: list[dict] | None = None) -> Image.Image:
    structure = components.snapshot(structure)
    canvas_info = structure.get("canvas", {})
    cw = int(canvas_info.get("width", 720))
    ch = int(canvas_info.get("height", 1560))

    # Resolve layout/align into concrete relative positions on every node.
    # render_element prefers `_rel` when present; this call populates it.
    layout_mod.resolve_positions(structure)

    canvas = Image.new("RGBA", (cw, ch), background)
    root = structure.get("root", {})
    render_element(canvas, root, 0, 0, assets, trace)
    return canvas


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--design", required=True, help="Path to the original design image")
    ap.add_argument("--structure", required=True, help="Path to ui_structure.json")
    ap.add_argument("--assets", required=True, help="Directory containing sliced PNG assets")
    ap.add_argument("--output", required=True, help="Path to save comparison.png")
    ap.add_argument("--reconstruction",
                    help="Optional path to save the raw reconstruction without a grid")
    ap.add_argument("--trace",
                    help="Optional path to save same-pass render diagnostics as JSON")
    ap.add_argument("--no-grid", action="store_true",
                    help="Disable the grid overlay on both panels. By default a "
                         "labeled grid is drawn on the design and the "
                         "reconstruction so positions can be verified against "
                         "the same ruler used in Step 1.")
    ap.add_argument("--background-color", default="#1E1E28",
                    help="Canvas background color for transparent areas "
                         "(default: #1E1E28).")
    ap.add_argument("--transparent-bg", action="store_true",
                    help="Render transparent areas with alpha 0. Useful when "
                         "the structure intentionally skips dimmed scene "
                         "content behind an active popup.")
    args = ap.parse_args()

    design_path = Path(args.design)
    structure_path = Path(args.structure)
    assets_dir = Path(args.assets)
    output_path = Path(args.output)

    if not design_path.exists():
        print(f"Design image not found: {design_path}", file=sys.stderr)
        sys.exit(1)
    if not structure_path.exists():
        print(f"Structure JSON not found: {structure_path}", file=sys.stderr)
        sys.exit(1)
    if not assets_dir.exists():
        print(f"Assets directory not found: {assets_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Loading structure: {structure_path}")
    structure = json.loads(structure_path.read_text(encoding="utf-8"))

    print(f"Indexing assets: {assets_dir}")
    assets = AssetCache(assets_dir)
    print(f"  {assets._file_count} image files indexed")
    print(f"  {assets._border_file_count} Unity sprite borders indexed")

    global FONT_SEARCH_DIRS
    FONT_SEARCH_DIRS = [
        path.resolve()
        for path in (
            structure_path.parent,
            structure_path.parent.parent,
            assets_dir,
            assets_dir.parent,
            design_path.parent,
            design_path.parent.parent,
        )
    ]

    if args.transparent_bg:
        background = (0, 0, 0, 0)
    else:
        background = hex_to_rgba(args.background_color) or (30, 30, 40, 255)

    print("Rendering reconstruction...")
    trace_entries: list[dict] | None = [] if args.trace else None
    reconstruction = render_from_structure(structure, assets, background, trace_entries)

    if args.reconstruction:
        reconstruction_path = Path(args.reconstruction)
        reconstruction_path.parent.mkdir(parents=True, exist_ok=True)
        reconstruction.save(reconstruction_path)
        print(f"Saved reconstruction: {reconstruction_path}")
    if args.trace:
        trace_path = Path(args.trace)
        trace_path.parent.mkdir(parents=True, exist_ok=True)
        trace_data = {
            "version": 1,
            "canvas": {
                "width": reconstruction.width,
                "height": reconstruction.height,
                "background": list(background),
            },
            "stats": {
                "elements": len(trace_entries or []),
                "texts": sum(1 for item in trace_entries or [] if item["type"] == "text"),
                "assets": sum(1 for item in trace_entries or [] if item.get("asset")),
            },
            "elements": trace_entries or [],
        }
        trace_path.write_text(
            json.dumps(trace_data, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"Saved render trace: {trace_path}")

    print(f"Loading design image: {design_path}")
    design_img = Image.open(design_path).convert("RGBA")
    canvas = structure.get("canvas") or {}
    cw, ch = canvas.get("width"), canvas.get("height")
    if cw and ch and (cw, ch) != design_img.size:
        print(
            f"  [warn] canvas in structure is {cw}x{ch} but design is "
            f"{design_img.width}x{design_img.height}; grid comparison is not "
            "pixel-accurate until the JSON uses the design's native canvas.",
            file=sys.stderr,
        )

    if not args.no_grid:
        # Same grid (cell ~= 45 px) on both panels so they can be cross-read
        print("Overlaying grid on both panels...")
        grid_palette = select_grid_palette(design_img)
        design_img = draw_grid(design_img, palette=grid_palette)
        reconstruction = draw_grid(reconstruction, palette=grid_palette)

    print("Building side-by-side comparison...")
    comparison = build_side_by_side(design_img, reconstruction)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    # Save as PNG (RGBA) and also a JPG-friendly version if requested
    comparison.convert("RGB").save(output_path)
    print(f"Saved: {output_path}")


if __name__ == "__main__":
    main()
