#!/usr/bin/env python3
"""
Rasterizes SVG icons to PNG.

Usage:
    python rasterize_svg_to_png.py [input.svg] [output.png] [--size N] [--sharp]

  --sharp         Rasterize at 2x then downscale with nearest-neighbor for crisp edges.
  --native-size WxH  Render at viewBox size (e.g. 37x43) then resize to --size; best for file icon.

Backends (first available is used):
  1. resvg_py  — pip install resvg_py (no system deps, recommended)
  2. cairosvg  — pip install cairosvg (on Windows may need Cairo/GTK runtime)
  3. Inkscape  — if installed (e.g. from https://inkscape.org)

Examples:
    python rasterize_svg_to_png.py
    python rasterize_svg_to_png.py UI4_big_material_icon.svg
    python rasterize_svg_to_png.py UI4_big_material_icon.svg ../Editor/Assets/ui/UI4_big_material_icon.png --size 64
"""

import argparse
import os
import shutil
import subprocess
import sys


def _downscale_nearest(png_path: str, target_size: int) -> bool:
    """Downscale PNG to target_size using nearest-neighbor for crisp edges."""
    try:
        from PIL import Image
    except ImportError:
        return False
    try:
        resample = getattr(Image.Resampling, "NEAREST", Image.NEAREST)
        with Image.open(png_path) as img:
            img = img.convert("RGBA")
            out = img.resize((target_size, target_size), resample)
            out.save(png_path)
        return True
    except Exception:
        return False


def rasterize_resvg(svg_path: str, png_path: str, size: int, render_scale: int = 1, native_size: tuple = None) -> bool:
    try:
        import resvg_py
    except ImportError:
        return False
    if native_size:
        nw, nh = native_size
        w, h = nw * render_scale, nh * render_scale
    else:
        w = h = (size * render_scale) if render_scale > 1 else size
    try:
        png_bytes = resvg_py.svg_to_bytes(
            svg_path=os.path.abspath(svg_path),
            width=w,
            height=h,
        )
        with open(os.path.abspath(png_path), "wb") as f:
            f.write(png_bytes)
        if (render_scale > 1 or native_size) and (w != size or h != size):
            _downscale_nearest(os.path.abspath(png_path), size)
        return True
    except Exception:
        return False


def rasterize_cairosvg(svg_path: str, png_path: str, size: int, render_scale: int = 1, native_size: tuple = None) -> bool:
    try:
        import cairosvg
    except ImportError:
        return False
    except OSError:
        return False  # e.g. cairo DLL not found on Windows
    if native_size:
        nw, nh = native_size
        w, h = nw * render_scale, nh * render_scale
    else:
        w = h = (size * render_scale) if render_scale > 1 else size
    try:
        cairosvg.svg2png(
            url=os.path.abspath(svg_path),
            write_to=os.path.abspath(png_path),
            output_width=w,
            output_height=h,
        )
        if (render_scale > 1 or native_size) and (w != size or h != size):
            _downscale_nearest(os.path.abspath(png_path), size)
        return True
    except Exception:
        return False


def _find_inkscape() -> str:
    ink = shutil.which("inkscape")
    if ink:
        return ink
    # Windows: common install paths
    for path in (
        os.environ.get("ProgramFiles", "C:\\Program Files") + "\\Inkscape\\bin\\inkscape.exe",
        os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)") + "\\Inkscape\\inkscape.exe",
    ):
        if os.path.isfile(path):
            return path
    return ""


def rasterize_inkscape(svg_path: str, png_path: str, size: int, render_scale: int = 1, native_size: tuple = None) -> bool:
    ink = _find_inkscape()
    if not ink:
        return False
    if native_size:
        nw, nh = native_size
        w, h = nw * render_scale, nh * render_scale
    else:
        w = h = (size * render_scale) if render_scale > 1 else size
    out = os.path.abspath(png_path)
    try:
        subprocess.run(
            [ink, str(svg_path), "-o", out, "-w", str(w), "-h", str(h)],
            check=True,
            capture_output=True,
        )
        if (render_scale > 1 or native_size) and (w != size or h != size):
            _downscale_nearest(out, size)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="Rasterize SVG to PNG")
    parser.add_argument(
        "input",
        nargs="?",
        default=os.path.join(script_dir, "UI4_big_material_icon.svg"),
        help="Input SVG path (default: UI4_big_material_icon.svg in script dir)",
    )
    parser.add_argument(
        "output",
        nargs="?",
        default=None,
        help="Output PNG path (default: same name as input, .png in script dir)",
    )
    parser.add_argument(
        "--size",
        "-s",
        type=int,
        default=64,
        help="Output width and height in pixels (default: 64)",
    )
    parser.add_argument(
        "--sharp",
        action="store_true",
        help="Render at 2x then downscale with nearest-neighbor for crisp edges",
    )
    parser.add_argument(
        "--native-size",
        metavar="WxH",
        default=None,
        help="Render at viewBox size (e.g. 37x43) then resize to --size; use for file icon",
    )
    args = parser.parse_args()
    render_scale = 2 if args.sharp else 1
    native_size = None
    if args.native_size:
        try:
            a, b = args.native_size.strip().lower().split("x")
            native_size = (int(a), int(b))
        except ValueError:
            print(f"Invalid --native-size: {args.native_size}", file=sys.stderr)
            sys.exit(1)

    input_path = args.input
    if not os.path.isabs(input_path):
        for base in (script_dir, os.getcwd()):
            candidate = os.path.join(base, input_path)
            if os.path.isfile(candidate):
                input_path = candidate
                break
        else:
            input_path = os.path.join(script_dir, input_path)
    else:
        input_path = os.path.normpath(input_path)

    if args.output:
        output_path = args.output
        if not os.path.isabs(output_path):
            output_path = os.path.normpath(os.path.join(os.getcwd(), output_path))
    else:
        base = os.path.splitext(os.path.basename(input_path))[0]
        output_path = os.path.join(script_dir, f"{base}.png")

    if not os.path.isfile(input_path):
        print(f"Input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)

    def do_rasterize():
        if rasterize_resvg(input_path, output_path, args.size, render_scale, native_size):
            return True
        if rasterize_cairosvg(input_path, output_path, args.size, render_scale, native_size):
            return True
        if rasterize_inkscape(input_path, output_path, args.size, render_scale, native_size):
            return True
        return False

    if not do_rasterize():
        print(
            "No backend available. Install: pip install resvg_py",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"OK: {output_path} ({args.size}x{args.size})")

    # Always save a copy next to the SVG for comparison
    input_dir = os.path.dirname(input_path)
    input_base = os.path.splitext(os.path.basename(input_path))[0]
    beside_path = os.path.join(input_dir, f"{input_base}.png")
    if os.path.normpath(beside_path) != os.path.normpath(output_path):
        shutil.copy2(output_path, beside_path)
        print(f"Copy: {beside_path}")


if __name__ == "__main__":
    main()
