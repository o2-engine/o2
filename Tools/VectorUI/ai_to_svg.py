#!/usr/bin/env python3
"""Converts the artboards of an Illustrator .ai file into flat SVGs of the engine subset.

    python3 ai_to_svg.py --ai UI4.ai --png-dir o2/Editor/Assets/ui --out <dir> [--only name,...]
"""

import argparse
import json
import os
import sys

import numpy as np

import ai_file
import effects
import fixes
import raw_svg
import render
import touch
from svg_model import Gradient, Group, Raster, Shape, as_axis_rect, count_primitives, iter_shapes, write_svg


def _visibility(page, index):
    above = [it for it in page.items[index + 1:] if not isinstance(it, Raster)]
    return 1.0 - render.render_items(page.w, page.h, above)[..., 3] / 255.0


def rebuild_effects(page, notes):
    """Replaces every Raster layer with fitted vector primitives; True when anything was rebuilt."""
    out, rebuilt = [], False
    for index, it in enumerate(page.items):
        if not isinstance(it, Raster):
            out.append(it)
            continue
        if it.rgba[..., 3].max() < 1.0:
            continue
        shapes, info = effects.fit_shadow(it, _visibility(page, index))
        out.extend(shapes)
        rebuilt = True
        notes.append('shadow: core %s reach %.1f peak %.3f stops %d fit-err %.1f' % (
            'x'.join(str(v) for v in info['core']), info['reach'], info['peak'], len(info['knots']), info['max_err']))
        if info['max_err'] > 6.0:
            page.flag('shadow is not that of a rounded rectangle, approximated (max error %.0f/255)' % info['max_err'])
    page.items = out
    return rebuilt


def snap_colors(page, ref, notes):
    """Flat colours that the PNG shows 1..2 levels away from the rounded Illustrator value take the PNG value."""
    img = np.rint(render.render_items(page.w, page.h, page.items))
    solid = (img[..., 3] == 255) & (ref[..., 3] == 255)
    remap = {}
    for s in iter_shapes(page.items):
        for paint in (s.fill, s.stroke):
            if not isinstance(paint, tuple):
                continue
            key = tuple(int(np.floor(c + 0.5)) for c in paint)
            if key in remap:
                continue
            remap[key] = key
            sel = solid & (img[..., 0] == key[0]) & (img[..., 1] == key[1]) & (img[..., 2] == key[2])
            if sel.sum() < 4:
                continue
            vals, counts = np.unique(ref[sel][:, :3].astype(int), axis=0, return_counts=True)
            mode = tuple(int(v) for v in vals[counts.argmax()])
            if mode == key:
                continue
            share = counts.max() / float(sel.sum())
            close = max(abs(a - b) for a, b in zip(mode, key)) <= 2
            if (close and share >= 0.6) or (share >= 0.9 and sel.sum() >= 8):
                remap[key] = mode
                notes.append('colour #%02x%02x%02x -> #%02x%02x%02x from PNG' % (key + mode))
                if not close:
                    page.flag('PNG colour #%02x%02x%02x differs from the artboard #%02x%02x%02x' % (mode + key))
    for s in iter_shapes(page.items):
        for attr in ('fill', 'stroke'):
            paint = getattr(s, attr)
            if isinstance(paint, tuple):
                setattr(s, attr, tuple(float(v) for v in remap[tuple(int(np.floor(c + 0.5)) for c in paint)]))


def cleanup(page):
    def keep(items):
        out = []
        for it in items:
            if isinstance(it, Group):
                it.children = keep(it.children)
                if it.children and it.opacity > 0.002:
                    out.append(it)
                continue
            box = it.bbox()
            if not it.visible() or box is None:
                continue
            if box[2] <= 0 or box[3] <= 0 or box[0] >= page.w or box[1] >= page.h:
                continue
            rect = as_axis_rect(it.cmds)
            if rect and it.rect is None and any(c[0] == 'Z' for c in it.cmds):
                it.rect = (rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1], 0.0)
            out.append(it)
        return out

    page.items = keep(page.items)


def _same_outline(a, b):
    pa = [c[-2:] for c in a.cmds if c[0] != 'Z']
    pb = [c[-2:] for c in b.cmds if c[0] != 'Z']
    return len(pa) == len(pb) and all(abs(p[0] - q[0]) + abs(p[1] - q[1]) < 1e-4 for p, q in zip(pa, pb))


def _flat_colour(paint):
    if isinstance(paint, tuple):
        return paint, 1.0
    colours = [stop[1] for stop in paint.stops]
    alphas = [stop[2] for stop in paint.stops]
    if max(max(abs(c[i] - colours[0][i]) for c in colours) for i in range(3)) > 1.5 or max(alphas) - min(alphas) > 0.03:
        return None, None
    return colours[0], sum(alphas)/len(alphas)


def flatten_groups(page, notes):
    """The engine multiplies a group opacity into the children, so overlapping children are made disjoint."""
    out = []
    for it in page.items:
        done = None
        if isinstance(it, Group) and len(it.children) == 2 and all(isinstance(c, Shape) for c in it.children):
            below, above = it.children
            same = _same_outline(below, above)
            if same and below.stroke is None and above.fill is None and above.stroke_opacity > 0.9995:
                cmds = touch.inset_cmds(below.cmds, above.stroke_width*0.5, below.fill_rule)
                if cmds is not None:
                    below.cmds, below.rect = cmds, None
                    below.fill_opacity *= it.opacity
                    above.stroke_opacity *= it.opacity
                    done = [below, above]
                    notes.append('group opacity %.2f: fill inset to the inner edge of the stroke' % it.opacity)
            elif same and below.stroke is None and above.stroke is None and below.fill_opacity > 0.9995:
                colour, alpha = _flat_colour(above.fill)
                if colour is not None:
                    k = alpha*above.fill_opacity

                    def mix(c):
                        return tuple(colour[i]*k + c[i]*(1.0 - k) for i in range(3))

                    if isinstance(below.fill, Gradient):
                        below.fill = Gradient(below.fill.kind, below.fill.coords,
                                              [(o, mix(c), a) for o, c, a in below.fill.stops], below.fill.transform)
                    else:
                        below.fill = mix(below.fill)
                    below.fill_opacity *= it.opacity
                    done = [below]
                    notes.append('group opacity %.2f: two fills blended into one' % it.opacity)
        if done is None:
            out.append(it)
        else:
            out.extend(done)
            page.flags[:] = [f for f in page.flags if 'group opacity' not in f]
    page.items = out


def classify(page, rebuilt):
    if page.flags:
        return 'flagged'
    if rebuilt:
        return 'shadow-reconstructed'
    shapes = list(iter_shapes(page.items))
    if any(isinstance(p, Gradient) for s in shapes for p in (s.fill, s.stroke)):
        return 'gradient'
    if any(isinstance(it, Group) for it in page.items) or \
            any(s.opacity < 0.9995 or s.fill_opacity < 0.9995 or s.stroke_opacity < 0.9995 for s in shapes):
        return 'opacity'
    return 'simple'


def convert_page(svg_text, name, ref, touch_amount=0.125):
    page = raw_svg.RawPage(svg_text)
    notes = []
    if ref is not None and ref.shape[:2] != (page.h, page.w):
        page.flag('PNG is %dx%d, artboard is %dx%d' % (ref.shape[1], ref.shape[0], page.w, page.h))
        ref = None
    rebuilt = rebuild_effects(page, notes)
    fixes.apply(name, page, notes)
    cleanup(page)
    flatten_groups(page, notes)
    if ref is not None:
        snap_colors(page, ref, notes)
    for s in iter_shapes(page.items):
        if s.linejoin != 'miter':
            s.miterlimit = 4.0
    if touch_amount > 0:
        touch.apply(iter_shapes(page.items), touch_amount)
    return page, classify(page, rebuilt), notes


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ai', required=True)
    ap.add_argument('--png-dir', required=True, help='editor sprites, the reference for colours and effects')
    ap.add_argument('--out', required=True)
    ap.add_argument('--only', default='', help='comma separated PNG base names')
    ap.add_argument('--cache', default=None, help='directory for the raw pdftocairo pages')
    ap.add_argument('--touch', type=float, default=0.125,
                    help='outward growth of fills that emulates the Illustrator export anti-aliasing, 0 to disable')
    args = ap.parse_args()

    only = set(n for n in args.only.split(',') if n)
    os.makedirs(args.out, exist_ok=True)
    manifest_path = os.path.join(args.out, 'manifest.json')
    manifest = {}
    if only and os.path.exists(manifest_path):
        with open(manifest_path, encoding='utf-8') as f:
            manifest = json.load(f)
    seen = set()
    for index, artboard in enumerate(ai_file.artboard_names(args.ai)):
        page_no = index + 1
        name = ai_file.png_name(page_no, artboard, seen)
        if name is None:
            continue
        seen.add(name)
        if only and name not in only:
            continue
        png = os.path.join(args.png_dir, name + '.png')
        if not os.path.exists(png):
            print('skip %s (page %d): no %s' % (artboard, page_no, png), file=sys.stderr)
            continue
        page, cls, notes = convert_page(ai_file.page_svg(args.ai, page_no, args.cache), name, render.load_png(png),
                                         args.touch)
        with open(os.path.join(args.out, name + '.svg'), 'w', encoding='utf-8') as f:
            f.write(write_svg(page.w, page.h, page.items))
        manifest[name] = {'page': page_no, 'class': cls, 'primitives': count_primitives(page.items),
                          'notes': page.flags + notes}
    with open(manifest_path, 'w', encoding='utf-8') as f:
        json.dump(dict(sorted(manifest.items())), f, indent=1, ensure_ascii=False)
        f.write('\n')
    print('%d sprites written to %s' % (len(manifest), args.out))


if __name__ == '__main__':
    main()
