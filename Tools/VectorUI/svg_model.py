"""Flat shape model for the engine SVG subset and its writer."""

import math


def fmt(v):
    s = ('%.3f' % v).rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s


def hex_color(rgb):
    return '#%02x%02x%02x' % tuple(max(0, min(255, int(math.floor(c + 0.5)))) for c in rgb)


def mat_mul(a, b):
    """a applied after b; matrices are (a, b, c, d, e, f) as in SVG."""
    return (a[0] * b[0] + a[2] * b[1], a[1] * b[0] + a[3] * b[1],
            a[0] * b[2] + a[2] * b[3], a[1] * b[2] + a[3] * b[3],
            a[0] * b[4] + a[2] * b[5] + a[4], a[1] * b[4] + a[3] * b[5] + a[5])


def mat_apply(m, x, y):
    return (m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5])


IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def transform_cmds(cmds, m):
    out = []
    for c in cmds:
        if c[0] == 'Z':
            out.append(c)
        else:
            pts = []
            for i in range(1, len(c), 2):
                pts.extend(mat_apply(m, c[i], c[i + 1]))
            out.append((c[0],) + tuple(pts))
    return out


def cmds_bbox(cmds):
    xs, ys = [], []
    for c in cmds:
        for i in range(1, len(c), 2):
            xs.append(c[i])
            ys.append(c[i + 1])
    if not xs:
        return None
    return (min(xs), min(ys), max(xs), max(ys))


def cmds_to_d(cmds):
    parts = []
    for c in cmds:
        parts.append(c[0] + ' '.join(fmt(v) for v in c[1:]))
    return ' '.join(parts)


def rect_cmds(x0, y0, x1, y1):
    return [('M', x0, y0), ('L', x1, y0), ('L', x1, y1), ('L', x0, y1), ('Z',)]


def round_rect_cmds(x0, y0, x1, y1, r):
    if r <= 0:
        return rect_cmds(x0, y0, x1, y1)
    k = r * 0.5522847498
    return [('M', x0 + r, y0), ('L', x1 - r, y0),
            ('C', x1 - r + k, y0, x1, y0 + r - k, x1, y0 + r), ('L', x1, y1 - r),
            ('C', x1, y1 - r + k, x1 - r + k, y1, x1 - r, y1), ('L', x0 + r, y1),
            ('C', x0 + r - k, y1, x0, y1 - r + k, x0, y1 - r), ('L', x0, y0 + r),
            ('C', x0, y0 + r - k, x0 + r - k, y0, x0 + r, y0), ('Z',)]


def as_axis_rect(cmds, eps=1e-4):
    """Returns (x0, y0, x1, y1) when the path is a single axis-aligned rectangle."""
    pts = []
    for c in cmds:
        if c[0] in ('M', 'L'):
            pts.append((c[1], c[2]))
        elif c[0] == 'C':
            return None
    if pts and len(pts) > 1 and abs(pts[0][0] - pts[-1][0]) < eps and abs(pts[0][1] - pts[-1][1]) < eps:
        pts.pop()
    if len(pts) != 4 or sum(1 for c in cmds if c[0] == 'M') != 1:
        return None
    xs = sorted(set(round(p[0], 4) for p in pts))
    ys = sorted(set(round(p[1], 4) for p in pts))
    if len(xs) != 2 or len(ys) != 2:
        return None
    for i in range(4):
        a, b = pts[i], pts[(i + 1) % 4]
        if abs(a[0] - b[0]) > eps and abs(a[1] - b[1]) > eps:
            return None
    return (xs[0], ys[0], xs[1], ys[1])


class Gradient:
    def __init__(self, kind, coords, stops, transform=None):
        self.kind = kind            # 'linear' (x1, y1, x2, y2) | 'radial' (cx, cy, r)
        self.coords = tuple(coords)
        self.stops = list(stops)    # (offset, (r, g, b) in 0..255, alpha 0..1)
        self.transform = transform

    def key(self):
        return (self.kind, tuple(fmt(c) for c in self.coords),
                tuple((fmt(o), hex_color(c), fmt(a)) for o, c, a in self.stops),
                None if self.transform is None else tuple(fmt(v) for v in self.transform))

    def transformed(self, m):
        """Bakes an affine matrix; a radial gradient keeps a gradientTransform unless m is a similarity."""
        lin = self.transform is None
        full = m if lin else mat_mul(m, self.transform)
        if self.kind == 'linear':
            x1, y1, x2, y2 = self.coords
            p1 = mat_apply(full, x1, y1)
            p2 = mat_apply(full, x2, y2)
            # isolines stay straight under any affine map: re-project the axis onto their normal
            ix, iy = -(y2 - y1), (x2 - x1)
            tx, ty = full[0] * ix + full[2] * iy, full[1] * ix + full[3] * iy
            nx, ny = -ty, tx
            nn = nx * nx + ny * ny
            if nn < 1e-18:
                return Gradient(self.kind, p1 + p2, self.stops)
            k = ((p2[0] - p1[0]) * nx + (p2[1] - p1[1]) * ny) / nn
            return Gradient(self.kind, (p1[0], p1[1], p1[0] + nx * k, p1[1] + ny * k), self.stops)
        cx, cy, r = self.coords
        sx = math.hypot(full[0], full[1])
        sy = math.hypot(full[2], full[3])
        dot = full[0] * full[2] + full[1] * full[3]
        if abs(sx - sy) < 1e-4 * max(sx, sy) and abs(dot) < 1e-4 * sx * sy:
            c = mat_apply(full, cx, cy)
            return Gradient(self.kind, (c[0], c[1], r * sx), self.stops)
        return Gradient(self.kind, self.coords, self.stops, full)


class Shape:
    def __init__(self, cmds):
        self.cmds = cmds
        self.fill = None            # None | (r, g, b) | Gradient
        self.fill_opacity = 1.0
        self.fill_rule = 'nonzero'
        self.stroke = None
        self.stroke_opacity = 1.0
        self.stroke_width = 1.0
        self.linecap = 'butt'
        self.linejoin = 'miter'
        self.miterlimit = 4.0
        self.opacity = 1.0
        self.rect = None            # (x, y, w, h, rx) emitted as <rect> instead of the path
        self.alpha_field = None     # pending per-pixel soft mask, resolved by the reader

    def bbox(self):
        b = cmds_bbox(self.cmds)
        if b is None:
            return None
        pad = self.stroke_width * 0.5 if self.stroke is not None else 0.0
        return (b[0] - pad, b[1] - pad, b[2] + pad, b[3] + pad)

    def visible(self):
        fill_on = self.fill is not None and self.fill_opacity > 0.002
        stroke_on = self.stroke is not None and self.stroke_opacity > 0.002 and self.stroke_width > 0
        return self.opacity > 0.002 and (fill_on or stroke_on)


class Group:
    def __init__(self, children, opacity=1.0):
        self.children = children
        self.opacity = opacity


class Raster:
    """A rasterised Illustrator effect, straight RGBA float image the size of the artboard."""

    def __init__(self, rgba):
        self.rgba = rgba


def iter_shapes(items):
    for it in items:
        if isinstance(it, Group):
            yield from iter_shapes(it.children)
        elif isinstance(it, Shape):
            yield it


def count_primitives(items):
    return sum(1 for _ in iter_shapes(items))


class _Defs:
    def __init__(self):
        self.ids = {}
        self.xml = []

    def ref(self, g):
        k = g.key()
        if k not in self.ids:
            gid = 'g%d' % len(self.ids)
            self.ids[k] = gid
            if g.kind == 'linear':
                x1, y1, x2, y2 = g.coords
                head = '<linearGradient id="%s" gradientUnits="userSpaceOnUse" x1="%s" y1="%s" x2="%s" y2="%s"' % (
                    gid, fmt(x1), fmt(y1), fmt(x2), fmt(y2))
                tail = '</linearGradient>'
            else:
                cx, cy, r = g.coords
                head = '<radialGradient id="%s" gradientUnits="userSpaceOnUse" cx="%s" cy="%s" r="%s"' % (
                    gid, fmt(cx), fmt(cy), fmt(r))
                tail = '</radialGradient>'
            if g.transform is not None:
                head += ' gradientTransform="matrix(%s)"' % ' '.join('%.6g' % v for v in g.transform)
            self.xml.append(head + '>')
            for off, col, a in g.stops:
                s = '<stop offset="%s" stop-color="%s"' % (fmt(off), hex_color(col))
                if a < 0.9995:
                    s += ' stop-opacity="%s"' % fmt(a)
                self.xml.append(s + '/>')
            self.xml.append(tail)
        return 'url(#%s)' % self.ids[k]


def _paint(p, defs):
    if p is None:
        return 'none'
    if isinstance(p, Gradient):
        return defs.ref(p)
    return hex_color(p)


def _shape_xml(s, defs):
    a = []
    if s.rect is not None:
        x, y, w, h, rx = s.rect
        a.append('<rect x="%s" y="%s" width="%s" height="%s"' % (fmt(x), fmt(y), fmt(w), fmt(h)))
        if rx > 0:
            a.append('rx="%s" ry="%s"' % (fmt(rx), fmt(rx)))
    else:
        a.append('<path d="%s"' % cmds_to_d(s.cmds))
    a.append('fill="%s"' % _paint(s.fill, defs))
    if s.fill is not None:
        if s.fill_opacity < 0.9995:
            a.append('fill-opacity="%s"' % fmt(s.fill_opacity))
        if s.fill_rule != 'nonzero':
            a.append('fill-rule="%s"' % s.fill_rule)
    if s.stroke is not None:
        a.append('stroke="%s" stroke-width="%s"' % (_paint(s.stroke, defs), fmt(s.stroke_width)))
        if s.stroke_opacity < 0.9995:
            a.append('stroke-opacity="%s"' % fmt(s.stroke_opacity))
        if s.linecap != 'butt':
            a.append('stroke-linecap="%s"' % s.linecap)
        if s.linejoin != 'miter':
            a.append('stroke-linejoin="%s"' % s.linejoin)
        elif abs(s.miterlimit - 4.0) > 1e-6:
            a.append('stroke-miterlimit="%s"' % fmt(s.miterlimit))
    if s.opacity < 0.9995:
        a.append('opacity="%s"' % fmt(s.opacity))
    return ' '.join(a) + '/>'


def _items_xml(items, defs, out):
    for it in items:
        if isinstance(it, Group):
            out.append('<g opacity="%s">' % fmt(it.opacity))
            _items_xml(it.children, defs, out)
            out.append('</g>')
        elif isinstance(it, Shape):
            out.append(_shape_xml(it, defs))


def write_svg(w, h, items):
    defs = _Defs()
    body = []
    _items_xml(items, defs, body)
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%s" height="%s" viewBox="0 0 %s %s">' % (
        fmt(w), fmt(h), fmt(w), fmt(h))]
    if defs.xml:
        out.append('<defs>')
        out.extend(defs.xml)
        out.append('</defs>')
    out.extend(body)
    out.append('</svg>')
    return '\n'.join(out) + '\n'
