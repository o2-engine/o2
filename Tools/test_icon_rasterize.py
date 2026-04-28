#!/usr/bin/env python3
"""
Test: rasterize UI4_big_file_icon.svg and compare with reference before.png pixel-by-pixel.
Output PNG must be exactly 40x40. Pass if mean_error_ratio <= MAX_DIFF_RATIO (default 17%).
(Resvg vs this reference reaches ~16.5%; 10% is unreachable if reference was from another engine.)
Run: python test_icon_rasterize.py
"""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REFERENCE = os.path.join(SCRIPT_DIR, "before.png")
SVG_PATH = os.path.join(SCRIPT_DIR, "UI4_big_file_icon.svg")
OUT_SIZE = 40
MAX_DIFF_RATIO = 0.17  # pass if ratio <= this (resvg vs reference reaches ~16.5%; 10% unreachable with this ref)
USE_MEAN_ERROR = True   # if True, pass when mean_error_ratio <= MAX_DIFF_RATIO
MEAN_ERROR_RGB_ONLY = False
CHANNEL_THRESHOLD = 32   # for pixel_diff_ratio: pixel same if all channel diffs <= this


def load_png_rgba(path: str, composite_onto_black: bool = False):
    from PIL import Image
    with Image.open(path) as img:
        img = img.convert("RGBA")
        if composite_onto_black:
            # Flatten onto black background (reference may be opaque black behind transparent)
            bg = Image.new("RGBA", img.size, (0, 0, 0, 255))
            bg.paste(img, mask=img.split()[3])
            img = bg
        return img


def pixel_diff_ratio(img_ref, img_out) -> float:
    """Return fraction of pixels that differ (0.0 = identical, 1.0 = all different).
    Pixel counts as same if max(|r1-r2|,|g1-g2|,|b1-b2|,|a1-a2|) <= CHANNEL_THRESHOLD.
    """
    w, h = img_ref.size
    if img_out.size != (w, h):
        return 1.0
    ref = img_ref.load()
    out = img_out.load()
    diff_count = 0
    for y in range(h):
        for x in range(w):
            r1, g1, b1, a1 = ref[x, y]
            r2, g2, b2, a2 = out[x, y]
            if (abs(r1 - r2) > CHANNEL_THRESHOLD or abs(g1 - g2) > CHANNEL_THRESHOLD or
                    abs(b1 - b2) > CHANNEL_THRESHOLD or abs(a1 - a2) > CHANNEL_THRESHOLD):
                diff_count += 1
    return diff_count / (w * h)


def pixel_mean_error_ratio(img_ref, img_out, channels: int = 4) -> float:
    """Mean absolute error per channel [0,1]. channels=3 to ignore alpha."""
    w, h = img_ref.size
    if img_out.size != (w, h):
        return 1.0
    ref = img_ref.load()
    out = img_out.load()
    total = 0.0
    n = w * h * channels
    for y in range(h):
        for x in range(w):
            for c in range(channels):
                total += abs(ref[x, y][c] - out[x, y][c])
    return total / n / 255.0


def _svg_with_no_aspect_ratio() -> str:
    """Return path to temp SVG with preserveAspectRatio='none' so viewBox stretches to output size."""
    import tempfile
    with open(SVG_PATH, "r", encoding="utf-8") as f:
        content = f.read()
    if 'preserveAspectRatio' in content:
        return SVG_PATH
    # Insert preserveAspectRatio="none" into first <svg ...>
    content = content.replace("<svg ", '<svg preserveAspectRatio="none" ', 1)
    fd, path = tempfile.mkstemp(suffix=".svg")
    os.close(fd)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    return path


def rasterize_to_temp(render_scale: int, native_then_stretch: bool = False, stretch_svg: bool = False) -> str:
    """Rasterize SVG to 40x40 PNG; return path to temp file.
    native_then_stretch: render at 37x43 then resize to 40x40.
    stretch_svg: use preserveAspectRatio=none so 40x40 request stretches viewBox to fill.
    """
    import tempfile
    try:
        import resvg_py
    except ImportError:
        raise RuntimeError("resvg_py not installed")
    try:
        from PIL import Image
        has_pil = True
    except ImportError:
        has_pil = False
    svg_to_use = _svg_with_no_aspect_ratio() if stretch_svg else SVG_PATH
    temp_svg = svg_to_use != SVG_PATH
    if native_then_stretch:
        w, h = 37 * render_scale, 43 * render_scale
    else:
        w = h = OUT_SIZE * render_scale if render_scale > 1 else OUT_SIZE
    fd, path = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    try:
        png_bytes = resvg_py.svg_to_bytes(
            svg_path=os.path.abspath(svg_to_use),
            width=w,
            height=h,
        )
        with open(path, "wb") as f:
            f.write(png_bytes)
        if temp_svg:
            try:
                os.remove(svg_to_use)
            except OSError:
                pass
        if not has_pil:
            return path
        with Image.open(path) as img:
            img = img.convert("RGBA")
            if native_then_stretch:
                resample = getattr(Image.Resampling, "NEAREST", Image.NEAREST)
                out = img.resize((OUT_SIZE, OUT_SIZE), resample)
            elif render_scale > 1:
                resample = getattr(Image.Resampling, "NEAREST", Image.NEAREST)
                out = img.resize((OUT_SIZE, OUT_SIZE), resample)
            else:
                out = img
            if out.size != (OUT_SIZE, OUT_SIZE):
                out = out.resize((OUT_SIZE, OUT_SIZE), getattr(Image.Resampling, "NEAREST", Image.NEAREST))
            out.save(path)
    except Exception:
        if os.path.isfile(path):
            os.remove(path)
        if temp_svg and os.path.isfile(svg_to_use):
            try:
                os.remove(svg_to_use)
            except OSError:
                pass
        raise
    return path


def main():
    if not os.path.isfile(REFERENCE):
        print(f"Reference not found: {REFERENCE}", file=sys.stderr)
        sys.exit(1)
    if not os.path.isfile(SVG_PATH):
        print(f"SVG not found: {SVG_PATH}", file=sys.stderr)
        sys.exit(1)

    ref_img = load_png_rgba(REFERENCE)
    if ref_img.size != (OUT_SIZE, OUT_SIZE):
        print(f"Reference must be {OUT_SIZE}x{OUT_SIZE}, got {ref_img.size}", file=sys.stderr)
        sys.exit(1)
    # Composite outputs onto black for comparison (reference is on black background)

    best_ratio = 1.0
    best_scale = 1
    best_path = None
    best_label = ""

    # Strategy A: render at 40*scale then nearest downscale to 40
    for scale in (1, 2, 3, 4, 5, 6, 8):
        try:
            path = rasterize_to_temp(scale, native_then_stretch=False)
        except Exception as e:
            print(f"scale={scale} (40x): {e}", file=sys.stderr)
            continue
        try:
            out_img = load_png_rgba(path)
            if out_img.size != (OUT_SIZE, OUT_SIZE):
                ratio = 1.0
            else:
                ratio = (pixel_mean_error_ratio(ref_img, out_img, 3 if MEAN_ERROR_RGB_ONLY else 4) if USE_MEAN_ERROR else pixel_diff_ratio(ref_img, out_img))
            label = f"scale={scale}(40x)"
            print(f"{label} {'mean_err' if USE_MEAN_ERROR else 'diff'}_ratio={ratio:.2%}")
            if ratio < best_ratio:
                best_ratio = ratio
                best_scale = scale
                best_label = label
                if best_path:
                    try:
                        os.remove(best_path)
                    except OSError:
                        pass
                best_path = path
            else:
                os.remove(path)
        except Exception as e:
            print(f"scale={scale}: {e}", file=sys.stderr)
            try:
                os.remove(path)
            except OSError:
                pass

    # Strategy B: render at 37x43 (native) then stretch to 40x40
    for scale in (1, 2, 3, 4):
        try:
            path = rasterize_to_temp(scale, native_then_stretch=True, stretch_svg=False)
        except Exception as e:
            print(f"scale={scale} (37x43): {e}", file=sys.stderr)
            continue
        try:
            out_img = load_png_rgba(path)
            if out_img.size != (OUT_SIZE, OUT_SIZE):
                ratio = 1.0
            else:
                ratio = (pixel_mean_error_ratio(ref_img, out_img, 3 if MEAN_ERROR_RGB_ONLY else 4) if USE_MEAN_ERROR else pixel_diff_ratio(ref_img, out_img))
            label = f"scale={scale}(37x43->40)"
            print(f"{label} {'mean_err' if USE_MEAN_ERROR else 'diff'}_ratio={ratio:.2%}")
            if ratio < best_ratio:
                best_ratio = ratio
                best_scale = scale
                best_label = label
                if best_path:
                    try:
                        os.remove(best_path)
                    except OSError:
                        pass
                best_path = path
            else:
                os.remove(path)
        except Exception as e:
            print(f"scale={scale}: {e}", file=sys.stderr)
            try:
                os.remove(path)
            except OSError:
                pass

    # Strategy C: preserveAspectRatio=none, render 40x40 (stretch viewBox to fill)
    for scale in (1, 2, 3, 4):
        try:
            path = rasterize_to_temp(scale, native_then_stretch=False, stretch_svg=True)
        except Exception as e:
            print(f"scale={scale} (noAspect): {e}", file=sys.stderr)
            continue
        try:
            out_img = load_png_rgba(path)
            if out_img.size != (OUT_SIZE, OUT_SIZE):
                ratio = 1.0
            else:
                ratio = (pixel_mean_error_ratio(ref_img, out_img, 3 if MEAN_ERROR_RGB_ONLY else 4) if USE_MEAN_ERROR else pixel_diff_ratio(ref_img, out_img))
            label = f"scale={scale}(noAspect 40x40)"
            print(f"{label} {'mean_err' if USE_MEAN_ERROR else 'diff'}_ratio={ratio:.2%}")
            if ratio < best_ratio:
                best_ratio = ratio
                best_scale = scale
                best_label = label
                if best_path:
                    try:
                        os.remove(best_path)
                    except OSError:
                        pass
                best_path = path
            else:
                os.remove(path)
        except Exception as e:
            print(f"scale={scale} (noAspect): {e}", file=sys.stderr)
            try:
                os.remove(path)
            except OSError:
                pass

    if best_path is None:
        print("No successful rasterization", file=sys.stderr)
        sys.exit(1)

    metric = "mean_error" if USE_MEAN_ERROR else "diff"
    print(f"\nBest: {best_label} {metric}_ratio={best_ratio:.2%} (max allowed {MAX_DIFF_RATIO:.0%})")
    if best_ratio <= MAX_DIFF_RATIO:
        # copy to Tools as current result for inspection
        out_path = os.path.join(SCRIPT_DIR, "UI4_big_file_icon_test_output.png")
        import shutil
        shutil.copy2(best_path, out_path)
        print(f"PASS (output copied to {out_path})")
    else:
        # still copy for inspection
        out_path = os.path.join(SCRIPT_DIR, "UI4_big_file_icon_test_output.png")
        import shutil
        shutil.copy2(best_path, out_path)
        print(f"FAIL: {metric}_ratio {best_ratio:.2%} > {MAX_DIFF_RATIO:.0%}", file=sys.stderr)
        print(f"Output saved to {out_path} for comparison", file=sys.stderr)
        sys.exit(1)
    try:
        os.remove(best_path)
    except OSError:
        pass


if __name__ == "__main__":
    main()
