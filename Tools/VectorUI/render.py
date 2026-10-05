"""Reference rendering (cairosvg) and image comparison helpers."""

import copy
import io
import os

import numpy as np
from PIL import Image

if os.path.isdir('/opt/homebrew/lib'):
    os.environ.setdefault('DYLD_FALLBACK_LIBRARY_PATH', '/opt/homebrew/lib')
import cairosvg  # noqa: E402

from svg_model import Group, Raster, Shape, cmds_to_d, rect_cmds, write_svg  # noqa: E402

OVERSCAN = 0.125
BACKGROUNDS = {'grey': (96, 96, 96), 'white': (255, 255, 255), 'black': (0, 0, 0)}


def render_svg(svg_text, scale=1):
    """Straight-alpha RGBA float array (0..255)."""
    png = cairosvg.svg2png(bytestring=svg_text.encode('utf-8'), scale=scale)
    return np.asarray(Image.open(io.BytesIO(png)).convert('RGBA')).astype(np.float64)


def load_png(path):
    return np.asarray(Image.open(path).convert('RGBA')).astype(np.float64)


def coverage(cmds, w, h, rule='nonzero', stroke_width=0.0, linejoin='round', fill=True):
    """Anti-aliased coverage (0..1) of a path, optionally widened by a stroke."""
    attrs = 'fill="%s" fill-rule="%s"' % ('#fff' if fill else 'none', rule)
    if stroke_width > 0:
        attrs += ' stroke="#fff" stroke-width="%.4f" stroke-linejoin="%s" stroke-linecap="round"' % (
            stroke_width, linejoin)
    svg = '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d"><path d="%s" %s/></svg>' % (
        w, h, w, h, cmds_to_d(cmds), attrs)
    return render_svg(svg)[..., 3] / 255.0


def over(dst, src):
    """Composites straight-alpha src over dst (both float 0..255 RGBA)."""
    sa = src[..., 3:4] / 255.0
    da = dst[..., 3:4] / 255.0
    oa = sa + da * (1 - sa)
    rgb = (src[..., :3] * sa + dst[..., :3] * da * (1 - sa)) / np.maximum(oa, 1e-9)
    return np.concatenate([rgb, oa * 255.0], axis=2)


def render_items(w, h, items):
    """Renders a mixed list: vector runs through cairosvg, Raster layers composited as they are."""
    out = np.zeros((h, w, 4))
    run = []
    for it in list(items) + [None]:
        if it is None or isinstance(it, Raster):
            if run:
                out = over(out, render_svg(write_svg(w, h, run)))
                run = []
            if it is not None:
                out = over(out, it.rgba)
        else:
            run.append(it)
    return out


def on_background(img, bg):
    a = img[..., 3:4] / 255.0
    return img[..., :3] * a + np.asarray(bg, dtype=np.float64) * (1 - a)


def compare(a, b, tolerances=(8, 24)):
    """Worst case over the three backgrounds: (similarity per tolerance, mean abs diff)."""
    if a.shape != b.shape:
        return None
    sims = [1.0] * len(tolerances)
    mad = 0.0
    for bg in BACKGROUNDS.values():
        d = np.abs(np.rint(on_background(a, bg)) - np.rint(on_background(b, bg))).max(axis=2)
        for i, t in enumerate(tolerances):
            sims[i] = min(sims[i], float((d <= t).mean()))
        mad = max(mad, float(d.mean()))
    return sims, mad


def diff_map(a, b, bg=BACKGROUNDS['grey']):
    return np.abs(on_background(a, bg) - on_background(b, bg)).max(axis=2)


def _pool(a, k, fn):
    h, w = a.shape[0] // k, a.shape[1] // k
    return fn(fn(a.reshape(h, k, w, k, *a.shape[2:]), axis=3), axis=1)


def _mask(w, h, shape, part, rule):
    s = copy.copy(shape)
    s.rect = None
    s.opacity = s.fill_opacity = s.stroke_opacity = 1.0
    s.fill = (255, 255, 255) if part == 'fill' else None
    s.stroke = (255, 255, 255) if part == 'stroke' else None
    if rule == 'overscan':
        # what a coverage rasteriser gives once fills are grown by OVERSCAN px
        if part == 'fill':
            s.stroke, s.stroke_width, s.linejoin, s.miterlimit = s.fill, 2 * OVERSCAN, 'miter', 4.0
        return render_svg(write_svg(w, h, [s]))[..., 3] / 255.0
    # Illustrator: a 1/4 px sub-pixel is on when the geometry touches it; strokes are one sub-pixel thinner
    if part == 'stroke':
        s.stroke_width = max(s.stroke_width - 2 * OVERSCAN, 0.01)
    fine = render_svg(write_svg(w, h, [s]), scale=16)[..., 3]
    return (_pool(fine, 4, np.max) > 0).astype(np.float64)


def _paint_layer(w, h, paint, mask, opacity):
    """Premultiplied layer of a paint under a mask, at the resolution of the mask."""
    if isinstance(paint, tuple):
        rgba = np.empty(mask.shape + (4,))
        rgba[..., :3] = np.rint(np.asarray(paint))
        rgba[..., 3] = 255.0
    else:
        s = Shape(rect_cmds(0, 0, w, h))
        s.fill = paint
        rgba = render_svg(write_svg(w, h, [s]), scale=mask.shape[0] // h)
    a = rgba[..., 3:4] / 255.0 * mask[..., None] * opacity
    return np.concatenate([rgba[..., :3] * a, a * 255.0], axis=2)


def _over_pm(dst, src):
    return src + dst * (1 - src[..., 3:4] / 255.0)


def _render_pm(w, h, items, rule):
    k = 1 if rule == 'overscan' else 4
    out = np.zeros((h * k, w * k, 4))
    for it in items:
        if isinstance(it, Raster):
            layer = np.kron(it.rgba, np.ones((k, k, 1)))
            layer[..., :3] *= layer[..., 3:4] / 255.0
        elif isinstance(it, Group):
            layer = _render_pm(w, h, it.children, rule) * it.opacity
        else:
            layer = np.zeros_like(out)
            if it.fill is not None:
                layer = _paint_layer(w, h, it.fill, _mask(w, h, it, 'fill', rule), it.fill_opacity)
            if it.stroke is not None and it.stroke_width > 0:
                layer = _over_pm(layer, _paint_layer(w, h, it.stroke, _mask(w, h, it, 'stroke', rule), it.stroke_opacity))
            layer = layer * it.opacity
        out = _over_pm(out, layer)
    return out


def render_items_rule(w, h, items, rule):
    """rule 'ai': emulates Illustrator's 'art optimized' export (4x4 sub-pixels, touch rule for fills).
    rule 'overscan': box-filter coverage with every fill grown by OVERSCAN px."""
    pm = _render_pm(w, h, items, rule)
    if rule == 'ai':
        pm = _pool(pm, 4, np.mean)
    a = pm[..., 3:4]
    return np.concatenate([np.clip(pm[..., :3] * 255.0 / np.maximum(a, 1e-9), 0, 255), a], axis=2)
