"""Reads the SVG that `pdftocairo -svg` writes for one artboard and flattens it into svg_model items."""

import base64
import io
import math
import re
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

import render
from svg_model import (IDENTITY, Gradient, Group, Raster, Shape, as_axis_rect, cmds_bbox, iter_shapes, mat_mul,
                       rect_cmds, transform_cmds, write_svg)

SVG_NS = '{http://www.w3.org/2000/svg}'
XLINK = '{http://www.w3.org/1999/xlink}href'
INHERITED = ('fill', 'fill-opacity', 'fill-rule', 'stroke', 'stroke-opacity', 'stroke-width', 'stroke-linecap',
             'stroke-linejoin', 'stroke-miterlimit', 'stroke-dasharray')
NUM = r'[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?'


def _tag(el):
    return el.tag.replace(SVG_NS, '')


def parse_transform(s):
    m = IDENTITY
    for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', s or ''):
        v = [float(x) for x in re.findall(NUM, args)]
        if name == 'matrix':
            t = tuple(v)
        elif name == 'translate':
            t = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0.0)
        elif name == 'scale':
            t = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
        else:
            raise ValueError('unsupported transform ' + name)
        m = mat_mul(m, t)
    return m


def parse_color(s):
    s = s.strip()
    m = re.match(r'rgb\(\s*(%s)%%\s*,\s*(%s)%%\s*,\s*(%s)%%\s*\)' % (NUM, NUM, NUM), s)
    if m:
        return tuple(float(m.group(i)) * 2.55 for i in (1, 2, 3))
    m = re.match(r'rgb\(\s*(%s)\s*,\s*(%s)\s*,\s*(%s)\s*\)' % (NUM, NUM, NUM), s)
    if m:
        return tuple(float(m.group(i)) for i in (1, 2, 3))
    if s.startswith('#') and len(s) == 7:
        return tuple(float(int(s[i:i + 2], 16)) for i in (1, 3, 5))
    raise ValueError('unsupported colour ' + s)


def parse_path(d):
    cmds = []
    for cmd, args in re.findall(r'([MLCZ])([^MLCZ]*)', d):
        v = [float(x) for x in re.findall(NUM, args)]
        if cmd == 'Z':
            cmds.append(('Z',))
        else:
            n = 6 if cmd == 'C' else 2
            for i in range(0, len(v), n):
                cmds.append((cmd if cmd != 'M' or i == 0 else 'L',) + tuple(v[i:i + n]))
    if re.search(r'[a-zHhVvSsQqTtAa]', d):
        raise ValueError('unsupported path command in ' + d[:60])
    while cmds and cmds[-1][0] == 'M':
        cmds.pop()
    return cmds


def simplify_stops(stops, tol=0.75):
    """Drops stops that a straight line between the kept neighbours reproduces within tol (0..255)."""
    def vec(s):
        return np.array([s[1][0], s[1][1], s[1][2], s[2] * 255.0])

    keep = [0, len(stops) - 1]

    def split(i0, i1):
        worst, wi = 0.0, None
        o0, o1 = stops[i0][0], stops[i1][0]
        v0, v1 = vec(stops[i0]), vec(stops[i1])
        for i in range(i0 + 1, i1):
            t = 0.0 if o1 == o0 else (stops[i][0] - o0) / (o1 - o0)
            e = float(np.abs(vec(stops[i]) - (v0 + (v1 - v0) * t)).max())
            if e > worst:
                worst, wi = e, i
        if wi is not None and worst > tol:
            keep.append(wi)
            split(i0, wi)
            split(wi, i1)

    if len(stops) > 2:
        split(0, len(stops) - 1)
    return [stops[i] for i in sorted(set(keep))]


class RawPage:
    def __init__(self, text):
        self.root = ET.fromstring(text)
        self.ids = {el.get('id'): el for el in self.root.iter() if el.get('id')}
        vb = [float(v) for v in self.root.get('viewBox').split()]
        self.w, self.h = int(math.ceil(vb[2] - 1e-6)), int(math.ceil(vb[3] - 1e-6))
        self.flags = []
        # an artboard of fractional height is exported from its bottom edge
        self.items = self._children(self.root, (1, 0, 0, 1, 0, self.h - vb[3]), {})
        if self.w - vb[2] > 0.01 or self.h - vb[3] > 0.01:
            self.flag('artboard is %gx%g, not whole pixels' % (vb[2], vb[3]))
        for shape in iter_shapes(self.items):
            if shape.alpha_field is not None:
                self._bake_alpha_field(shape)

    def flag(self, text):
        if text not in self.flags:
            self.flags.append(text)

    def _ref(self, value):
        m = re.match(r'url\(#([^)]+)\)', value)
        return self.ids[m.group(1)]

    def _children(self, el, ctm, style):
        out = []
        for ch in el:
            out.extend(self._node(ch, ctm, style))
        return out

    def _node(self, el, ctm, style):
        tag = _tag(el)
        if any(el.get(k) is not None for k in INHERITED):
            style = dict(style)
            style.update((k, el.get(k)) for k in INHERITED if el.get(k) is not None)
        if tag in ('defs', 'clipPath', 'mask', 'filter', 'linearGradient', 'radialGradient'):
            return []
        if el.get('transform'):
            ctm = mat_mul(ctm, parse_transform(el.get('transform')))
        if tag == 'g':
            items = self._children(el, ctm, style)
        elif tag == 'use':
            x, y = float(el.get('x', 0)), float(el.get('y', 0))
            items = self._node(self.ids[el.get(XLINK)[1:]], mat_mul(ctm, (1, 0, 0, 1, x, y)), style)
        elif tag == 'path':
            items = [self._shape(el, style, parse_path(el.get('d')), ctm)]
        elif tag == 'rect':
            x, y = float(el.get('x', 0)), float(el.get('y', 0))
            w, h = float(el.get('width')), float(el.get('height'))
            items = [self._shape(el, style, rect_cmds(x, y, x + w, y + h), ctm)]
        elif tag == 'image':
            items = [Raster(self._image(el, ctm))]
        else:
            self.flag('unsupported element <%s>' % tag)
            return []
        if el.get('filter'):
            self.flag('filter outside a mask')
        if tag == 'g' and el.get('opacity'):
            items = self._apply_opacity(items, float(el.get('opacity')))
        if el.get('mask'):
            items = self._apply_mask(items, self._ref(el.get('mask')), ctm)
        if el.get('clip-path'):
            items = self._apply_clip(items, self._ref(el.get('clip-path')), ctm)
        return items

    def _paint(self, value, ctm):
        if value is None or value == 'none':
            return None
        if value.startswith('url('):
            g = self._ref(value)
            stops = []
            for st in g:
                stops.append((float(st.get('offset')), parse_color(st.get('stop-color')),
                              float(st.get('stop-opacity', 1))))
            stops = simplify_stops(stops)
            gt = parse_transform(g.get('gradientTransform'))
            if g.get('gradientUnits') != 'userSpaceOnUse':
                self.flag('gradient not in userSpaceOnUse')
            if _tag(g) == 'linearGradient':
                grad = Gradient('linear', [float(g.get(k, 0)) for k in ('x1', 'y1', 'x2', 'y2')], stops)
            else:
                if abs(float(g.get('fx', g.get('cx', 0))) - float(g.get('cx', 0))) > 1e-6 or \
                        abs(float(g.get('fy', g.get('cy', 0))) - float(g.get('cy', 0))) > 1e-6:
                    self.flag('radial gradient with a focal point')
                grad = Gradient('radial', [float(g.get(k, 0)) for k in ('cx', 'cy', 'r')], stops)
            return grad.transformed(mat_mul(ctm, gt))
        return parse_color(value)

    def _shape(self, el, style, cmds, ctm):
        get = style.get
        s = Shape(transform_cmds(cmds, ctm))
        s.fill = self._paint(get('fill', 'rgb(0,0,0)'), ctm)
        s.fill_opacity = float(get('fill-opacity', 1))
        s.fill_rule = get('fill-rule', 'nonzero')
        s.stroke = self._paint(get('stroke'), ctm)
        s.stroke_opacity = float(get('stroke-opacity', 1))
        s.linecap = get('stroke-linecap', 'butt')
        s.linejoin = get('stroke-linejoin', 'miter')
        s.miterlimit = float(get('stroke-miterlimit', 4))
        if get('stroke-dasharray', 'none') != 'none':
            self.flag('dashed stroke dropped')
        sx = (ctm[0] ** 2 + ctm[1] ** 2) ** 0.5
        sy = (ctm[2] ** 2 + ctm[3] ** 2) ** 0.5
        if s.stroke is not None and abs(sx - sy) > 1e-4:
            self.flag('stroke under a non-uniform transform')
        s.stroke_width = float(get('stroke-width', 1)) * (sx * sy) ** 0.5
        s.opacity = float(el.get('opacity', 1))
        return s

    def _image(self, el, ctm):
        data = el.get(XLINK)
        img = Image.open(io.BytesIO(base64.b64decode(data.split(',', 1)[1]))).convert('RGBA')
        x, y = float(el.get('x', 0)), float(el.get('y', 0))
        w, h = float(el.get('width')), float(el.get('height'))
        m = mat_mul(ctm, (w / img.width, 0, 0, h / img.height, x, y))
        det = m[0] * m[3] - m[1] * m[2]
        inv = (m[3] / det, -m[2] / det, (m[2] * m[5] - m[3] * m[4]) / det,
               -m[1] / det, m[0] / det, (m[1] * m[4] - m[0] * m[5]) / det)
        # premultiplied resampling keeps transparent texels from bleeding colour
        a = np.asarray(img).astype(np.float64)
        a[..., :3] *= a[..., 3:4] / 255.0
        chans = [Image.fromarray(a[..., i].astype(np.float32), 'F').transform(
            (self.w, self.h), Image.AFFINE, inv, resample=Image.BILINEAR) for i in range(4)]
        out = np.stack([np.asarray(c, dtype=np.float64) for c in chans], axis=2)
        alpha = np.maximum(out[..., 3:4], 1e-9)
        out[..., :3] = np.clip(out[..., :3] * 255.0 / alpha, 0, 255)
        return out

    def _mask_alpha(self, mask, ctm):
        """A constant (float) or a per-pixel alpha array for the two mask forms pdftocairo writes."""
        inner = list(mask)
        if len(inner) == 1 and _tag(inner[0]) == 'g' and 'filter-remove-color' in inner[0].get('filter', ''):
            content = list(inner[0])
            if len(content) == 1 and _tag(content[0]) == 'rect':
                return float(content[0].get('fill-opacity', 1))
            if len(content) == 1 and _tag(content[0]) == 'use' and _tag(self.ids[content[0].get(XLINK)[1:]]) == 'image':
                use = content[0]
                m = mat_mul(ctm, parse_transform(use.get('transform')))
                img = self._image(self.ids[use.get(XLINK)[1:]], m)
                if 'color-to-alpha' not in use.get('filter', ''):
                    return img[..., 3] / 255.0
                lum = (0.2126 * img[..., 0] + 0.7152 * img[..., 1] + 0.0722 * img[..., 2]) / 255.0
                return lum * (img[..., 3] > 0)
        self.flag('unsupported mask form')
        return 1.0

    def _apply_mask(self, items, mask, ctm):
        alpha = self._mask_alpha(mask, ctm)
        if isinstance(alpha, float):
            return self._apply_opacity(items, alpha)
        out = []
        for it in items:
            if isinstance(it, Raster):
                it.rgba[..., 3] *= alpha
                out.append(it)
            elif isinstance(it, Shape) and it.stroke is None:
                it.alpha_field = alpha if it.alpha_field is None else it.alpha_field * alpha
                out.append(it)
            else:
                self.flag('image mask over stroked or grouped vector content ignored')
                out.append(it)
        return out

    def _apply_opacity(self, items, alpha):
        if alpha >= 0.9995:
            return items
        shapes = [it for it in items if not isinstance(it, Raster)]
        for it in items:
            if isinstance(it, Raster):
                it.rgba[..., 3] *= alpha
        simple = all(isinstance(s, Shape) and (s.fill is None or s.stroke is None) for s in shapes)
        if simple:
            boxes = [s.bbox() for s in shapes]
            for i in range(len(boxes)):
                for j in range(i):
                    a, b = boxes[i], boxes[j]
                    if a and b and a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]:
                        simple = False
        if simple:
            for s in shapes:
                s.fill_opacity *= alpha
                s.stroke_opacity *= alpha
            return items
        if len(shapes) == 1 and isinstance(shapes[0], Shape):
            shapes[0].opacity *= alpha
            return items
        self.flag('group opacity over overlapping shapes kept as <g opacity>')
        out, run = [], []
        for it in items:
            if isinstance(it, Raster):
                if run:
                    out.append(Group(run, alpha))
                    run = []
                out.append(it)
            else:
                run.append(it)
        if run:
            out.append(Group(run, alpha))
        return out

    def _apply_clip(self, items, clip, ctm):
        regions = []
        for ch in clip:
            m = mat_mul(ctm, parse_transform(ch.get('transform')))
            if _tag(ch) == 'rect':
                x, y = float(ch.get('x', 0)), float(ch.get('y', 0))
                cmds = rect_cmds(x, y, x + float(ch.get('width')), y + float(ch.get('height')))
            elif _tag(ch) == 'path':
                cmds = parse_path(ch.get('d'))
            else:
                self.flag('unsupported clipPath content')
                return items
            regions.append((transform_cmds(cmds, m), ch.get('clip-rule', 'nonzero')))
        if len(regions) != 1:
            self.flag('clipPath with %d children' % len(regions))
            return items
        cmds, rule = regions[0]
        return [self._clip_item(it, cmds, rule) for it in items]

    def _clip_item(self, it, cmds, rule):
        eps = 0.01
        crect = as_axis_rect(cmds)
        if isinstance(it, Group):
            it.children = [self._clip_item(c, cmds, rule) for c in it.children]
            return it
        if isinstance(it, Raster):
            if crect and crect[0] <= eps and crect[1] <= eps and crect[2] >= self.w - eps and crect[3] >= self.h - eps:
                return it
            it.rgba[..., 3] *= render.coverage(cmds, self.w, self.h, rule)
            return it
        box = it.bbox()
        if box is None:
            return it
        vis = (max(box[0], 0.0), max(box[1], 0.0), min(box[2], float(self.w)), min(box[3], float(self.h)))
        if crect:
            if crect[0] <= vis[0] + eps and crect[1] <= vis[1] + eps and crect[2] >= vis[2] - eps and crect[3] >= vis[3] - eps:
                return it
            srect = as_axis_rect(it.cmds)
            if not srect and it.stroke is None and _convex_contains(it.cmds, crect, eps):
                it.cmds = rect_cmds(*crect)
                return it
            if srect and it.stroke is None:
                x0, y0 = max(srect[0], crect[0]), max(srect[1], crect[1])
                x1, y1 = min(srect[2], crect[2]), min(srect[3], crect[3])
                it.cmds = rect_cmds(x0, y0, max(x0, x1), max(y0, y1))
                return it
        if it.stroke is None and _convex_contains(it.cmds, cmds_bbox(cmds), eps):
            it.cmds = list(cmds)
            it.fill_rule = rule
            return it
        if crect and self._outside_alpha(it, crect) < 2.0:
            return it
        self.flag('clip cuts a shape that does not simply cover it (left unclipped)')
        return it

    def _outside_alpha(self, shape, crect):
        img = render.render_svg(write_svg(self.w, self.h, [shape]), scale=4)[..., 3]
        x0, y0 = int(np.floor(crect[0] * 4 + 0.01)), int(np.floor(crect[1] * 4 + 0.01))
        x1, y1 = int(np.ceil(crect[2] * 4 - 0.01)), int(np.ceil(crect[3] * 4 - 0.01))
        img[max(y0, 0):max(y1, 0), max(x0, 0):max(x1, 0)] = 0
        return float(img.max())

    def _bake_alpha_field(self, shape):
        """Turns a soft mask that varies only along one axis into stop-opacity of a linear gradient."""
        field, shape.alpha_field = shape.alpha_field, None
        inside = render.coverage(shape.cmds, self.w, self.h, shape.fill_rule) > 0.99
        if not inside.any():
            shape.fill = None
            return
        ys, xs = np.nonzero(inside)
        px, py, vals = xs + 0.5, ys + 0.5, field[inside]
        if vals.max() - vals.min() < 1.5 / 255:
            shape.fill_opacity *= float(vals.mean())
            return
        box = cmds_bbox(shape.cmds)
        if isinstance(shape.fill, Gradient) and shape.fill.kind == 'linear':
            axes = [shape.fill.coords]
        elif isinstance(shape.fill, Gradient):
            self.flag('soft mask over a radial gradient ignored')
            return
        else:
            axes = [(box[0], 0, box[2], 0), (0, box[1], 0, box[3])]
        best = None
        for x1, y1, x2, y2 in axes:
            dx, dy = x2 - x1, y2 - y1
            t = np.clip(((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy), 0, 1)
            edges = np.linspace(0, 1, 65)
            idx = np.minimum((t * 64).astype(int), 63)
            sums = np.bincount(idx, vals, 64)
            cnt = np.bincount(idx, None, 64)
            have = cnt > 0
            centres = (edges[:-1] + edges[1:]) / 2
            prof_t, prof_a = centres[have], sums[have] / cnt[have]
            err = float(np.abs(np.interp(t, prof_t, prof_a) - vals).max())
            if best is None or err < best[0]:
                best = (err, (x1, y1, x2, y2), prof_t, prof_a)
        err, axis, prof_t, prof_a = best
        if err > 8.0 / 255:
            self.flag('soft mask is not a ramp along one axis (max error %.0f/255)' % (err * 255))
        if isinstance(shape.fill, Gradient):
            col_t = [s[0] for s in shape.fill.stops]
            cols = np.array([s[1] for s in shape.fill.stops])
            base_a = np.array([s[2] for s in shape.fill.stops])
        else:
            col_t, cols, base_a = [0.0, 1.0], np.array([shape.fill, shape.fill]), np.array([1.0, 1.0])
        offs = sorted(set([0.0, 1.0] + list(col_t) + [float(v) for v in prof_t]))
        stops = [(o, tuple(float(np.interp(o, col_t, cols[:, c])) for c in range(3)),
                  float(np.interp(o, col_t, base_a) * np.interp(o, prof_t, prof_a))) for o in offs]
        shape.fill = Gradient('linear', axis, simplify_stops(stops, 1.0))


def _convex_contains(poly_cmds, box, eps):
    """True when a single straight-edged convex contour contains the whole box."""
    if box is None or any(c[0] == 'C' for c in poly_cmds) or sum(1 for c in poly_cmds if c[0] == 'M') != 1:
        return False
    pts = [(c[1], c[2]) for c in poly_cmds if c[0] in ('M', 'L')]
    sign = 0
    for i in range(len(pts)):
        ax, ay = pts[i]
        bx, by = pts[(i + 1) % len(pts)]
        for cx, cy in ((box[0], box[1]), (box[2], box[1]), (box[2], box[3]), (box[0], box[3])):
            cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
            if abs(cross) <= eps * (abs(bx - ax) + abs(by - ay) + 1):
                continue
            if sign == 0:
                sign = 1 if cross > 0 else -1
            elif (cross > 0) != (sign > 0):
                return False
    return True
