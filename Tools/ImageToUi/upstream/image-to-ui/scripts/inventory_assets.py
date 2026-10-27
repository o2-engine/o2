"""
Inventory sliced UI assets before authoring ui_structure.json.

Outputs:
  - assets_inventory.json: relative paths, sizes, alpha bounds, duplicate
    basenames, Unity sprite borders, and simple usage hints.
  - assets_contact_sheet.png: all thumbnails with labels for visual picking.
  - assets_contact_sheet_usage_*.png: smaller sheets grouped by likely usage.

Usage:
    py -B inventory_assets.py --assets path/to/sprite_dir --output out/assets
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont


IMAGE_EXTS = {".png", ".jpg", ".jpeg"}


def find_font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
    ]
    for c in candidates:
        if Path(c).exists():
            try:
                return ImageFont.truetype(c, size)
            except Exception:
                continue
    return ImageFont.load_default()


def read_unity_sprite_border(path: Path) -> dict[str, int] | None:
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


def likely_usage(rel_path: str, border: dict[str, int] | None) -> str:
    name = rel_path.lower()
    if any(k in name for k in ("character", "avatar", "portrait", "profile")):
        return "portrait"
    if any(k in name for k in ("itemicon", "icon_", "icon-", "/icon", "icon ")):
        return "icon"
    if any(k in name for k in ("shadow", "glow", "light", "focus")):
        return "overlay"
    if border or any(k in name for k in (
        "button", "btn", "popup", "panel", "panal", "frame", "bg", "bar",
        "bubble", "border", "slot",
    )):
        return "button_or_panel"
    return "image"


def alpha_bbox(img: Image.Image) -> list[int]:
    rgba = img.convert("RGBA")
    bbox = rgba.getchannel("A").getbbox()
    if bbox is None:
        return [0, 0, 0, 0]
    return [int(v) for v in bbox]


def alpha_geometry(img: Image.Image) -> dict[str, Any]:
    """Separate texture bounds, faint effects, and the more opaque artwork."""
    alpha = img.convert("RGBA").getchannel("A")
    bounds = alpha.getbbox()
    bbox = list(bounds) if bounds else [0, 0, 0, 0]
    left, top, right, bottom = bbox
    core = alpha.point([0] * 128 + [255] * 128).getbbox()
    return {
        "alpha_bbox": bbox,
        "alpha_size": [right - left, bottom - top],
        "alpha_padding": (
            {"left": left, "top": top, "right": img.width - right,
             "bottom": img.height - bottom} if bounds else None
        ),
        "alpha_core_bbox": list(core) if core else None,
        "alpha_core_threshold": 128,
        "fully_transparent": bounds is None,
    }


def collect_assets(assets_dir: Path) -> dict[str, Any]:
    paths = sorted(
        (p for p in assets_dir.rglob("*")
         if p.is_file() and p.suffix.lower() in IMAGE_EXTS),
        key=lambda p: p.relative_to(assets_dir).as_posix().lower(),
    )
    by_basename: dict[str, list[str]] = defaultdict(list)
    items = []

    for path in paths:
        rel = path.relative_to(assets_dir).as_posix()
        by_basename[path.name.lower()].append(rel)
        try:
            with Image.open(path) as img:
                width, height = img.size
                geometry = alpha_geometry(img)
        except Exception as exc:
            print(f"  [warn] failed to read {rel}: {exc}", file=sys.stderr)
            continue
        border = read_unity_sprite_border(path)
        items.append({
            "path": rel,
            "basename": path.name,
            "width": width,
            "height": height,
            **geometry,
            "has_meta_border": border is not None,
            "spriteBorder": border,
            "likely_usage": likely_usage(rel, border),
        })

    duplicates = {
        basename: rels
        for basename, rels in sorted(by_basename.items())
        if len(rels) > 1
    }
    return {
        "assets_dir": str(assets_dir),
        "total_images": len(items),
        "meta_border_count": sum(1 for item in items if item["has_meta_border"]),
        "duplicate_basenames": duplicates,
        "assets": items,
    }


def fit_text(draw: ImageDraw.ImageDraw, text: str, font: ImageFont.FreeTypeFont,
             max_width: int) -> str:
    if draw.textlength(text, font=font) <= max_width:
        return text
    ellipsis = "..."
    out = text
    while out and draw.textlength(out + ellipsis, font=font) > max_width:
        out = out[1:]
    return ellipsis + out if out else ellipsis


def safe_slug(value: str) -> str:
    slug = re.sub(r"[^a-zA-Z0-9_-]+", "_", value.strip().lower())
    return slug.strip("_") or "group"


def make_contact_sheet(assets_dir: Path, items: list[dict[str, Any]],
                       output_path: Path, thumb_size: int,
                       columns: int) -> None:
    if not items:
        sheet = Image.new("RGB", (640, 160), "white")
        ImageDraw.Draw(sheet).text((20, 20), "No image assets found", fill=(0, 0, 0))
        sheet.save(output_path)
        return

    columns = max(1, columns)
    cell_w = max(220, thumb_size + 80)
    cell_h = thumb_size + 86
    rows = (len(items) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * cell_w, rows * cell_h), (248, 248, 248))
    draw = ImageDraw.Draw(sheet)
    label_font = find_font(13)
    small_font = find_font(11)

    for idx, item in enumerate(items):
        x = (idx % columns) * cell_w
        y = (idx // columns) * cell_h
        draw.rectangle([x, y, x + cell_w - 1, y + cell_h - 1],
                       outline=(215, 215, 215))
        path = assets_dir / item["path"]
        try:
            img = Image.open(path).convert("RGBA")
            img.thumbnail((thumb_size, thumb_size), Image.LANCZOS)
            checker = Image.new("RGBA", img.size, (238, 238, 238, 255))
            tile = 8
            cdraw = ImageDraw.Draw(checker)
            for cy in range(0, img.height, tile):
                for cx in range(0, img.width, tile):
                    if (cx // tile + cy // tile) % 2:
                        cdraw.rectangle([cx, cy, cx + tile - 1, cy + tile - 1],
                                        fill=(220, 220, 220, 255))
            checker.alpha_composite(img)
            px = x + (cell_w - checker.width) // 2
            py = y + 8
            sheet.paste(checker.convert("RGB"), (px, py))
        except Exception as exc:
            draw.text((x + 8, y + 18), f"read failed: {exc}", fill=(150, 0, 0),
                      font=small_font)

        label = fit_text(draw, item["path"], label_font, cell_w - 16)
        detail = f'{item["width"]}x{item["height"]}  {item["likely_usage"]}'
        border = "border" if item["has_meta_border"] else "no-border"
        draw.text((x + 8, y + thumb_size + 14), label, fill=(20, 20, 20),
                  font=label_font)
        draw.text((x + 8, y + thumb_size + 34), detail, fill=(70, 70, 70),
                  font=small_font)
        draw.text((x + 8, y + thumb_size + 52), border, fill=(95, 95, 95),
                  font=small_font)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)


def group_items(items: list[dict[str, Any]], mode: str) -> dict[str, list[dict[str, Any]]]:
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in items:
        if mode == "usage":
            key = item.get("likely_usage") or "image"
        else:
            parts = item["path"].split("/")
            key = parts[0] if len(parts) > 1 else "root"
        groups[key].append(item)
    return dict(sorted(groups.items(), key=lambda kv: kv[0].lower()))


def write_contact_sheets(assets_dir: Path, inventory: dict[str, Any],
                         out_dir: Path, thumb_size: int, columns: int,
                         split_mode: str) -> list[str]:
    items = inventory["assets"]
    sheet_paths: list[str] = []
    all_sheet = out_dir / "assets_contact_sheet.png"
    make_contact_sheet(assets_dir, items, all_sheet, thumb_size, columns)
    sheet_paths.append(all_sheet.name)

    split_modes = []
    if split_mode in ("usage", "both"):
        split_modes.append("usage")
    if split_mode in ("folder", "both"):
        split_modes.append("folder")

    for mode in split_modes:
        for group_name, group in group_items(items, mode).items():
            if len(group) == len(items):
                continue
            path = out_dir / f"assets_contact_sheet_{mode}_{safe_slug(group_name)}.png"
            make_contact_sheet(assets_dir, group, path, thumb_size, columns)
            sheet_paths.append(path.name)
    return sheet_paths


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--assets", required=True, help="Directory containing sliced PNG assets")
    ap.add_argument("--output", required=True,
                    help="Output directory for inventory JSON and contact sheet")
    ap.add_argument("--thumb-size", type=int, default=120,
                    help="Maximum thumbnail size in the contact sheet")
    ap.add_argument("--columns", type=int, default=4,
                    help="Contact sheet column count")
    ap.add_argument("--split-sheets", choices=("none", "usage", "folder", "both"),
                    default="usage",
                    help="Generate additional contact sheets by likely usage, "
                         "top-level folder, both, or none")
    ap.add_argument("--verbose", action="store_true",
                    help="Print every generated contact sheet and duplicate "
                         "basename. Default stdout is compact.")
    args = ap.parse_args()

    assets_dir = Path(args.assets)
    out_dir = Path(args.output)
    if not assets_dir.exists():
        print(f"Assets directory not found: {assets_dir}", file=sys.stderr)
        sys.exit(1)

    inventory = collect_assets(assets_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    json_path = out_dir / "assets_inventory.json"
    sheet_paths = write_contact_sheets(
        assets_dir,
        inventory,
        out_dir,
        args.thumb_size,
        args.columns,
        args.split_sheets,
    )
    inventory["contact_sheets"] = sheet_paths
    json_path.write_text(
        json.dumps(inventory, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    duplicate_count = len(inventory["duplicate_basenames"])
    print(f"Wrote {json_path}")
    print(f"  contact sheets: {len(sheet_paths)} in {out_dir}")
    print(f"  image assets: {inventory['total_images']}")
    print(f"  Unity sprite borders: {inventory['meta_border_count']}")
    print(f"  duplicate basenames: {duplicate_count}")
    if args.verbose:
        for sheet_name in sheet_paths:
            print(f"  sheet: {out_dir / sheet_name}")
    if args.verbose and inventory["duplicate_basenames"]:
        for basename, rels in inventory["duplicate_basenames"].items():
            print(f"  duplicate: {basename}: {', '.join(rels)}")


if __name__ == "__main__":
    main()
