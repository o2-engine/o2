"""Emulation of the Illustrator export anti-aliasing in geometry.

The editor PNGs were exported on a 4x4 sub-pixel grid where a sub-pixel is on when a fill touches it. For a fill
that is the Minkowski sum of the shape with a quarter-pixel square (every outline piece moves outwards by
`amount` along x and along y), limited by the quarter-pixel grid lines the shape reaches: an edge or an extremum
lying on that grid does not move. Strokes are left alone.
"""

import math

GRID = 0.25
GRID_EPS = 0.003
AXIS_EPS = 1e-4
EPS = 1e-9


def _lerp(a, b, t):
    return (a[0] + (b[0] - a[0])*t, a[1] + (b[1] - a[1])*t)


def _cubic_point(c, t):
    ab, bc, cd = _lerp(c[0], c[1], t), _lerp(c[1], c[2], t), _lerp(c[2], c[3], t)
    return _lerp(_lerp(ab, bc, t), _lerp(bc, cd, t), t)


def _cubic_split(c, t):
    ab, bc, cd = _lerp(c[0], c[1], t), _lerp(c[1], c[2], t), _lerp(c[2], c[3], t)
    abc, bcd = _lerp(ab, bc, t), _lerp(bc, cd, t)
    mid = _lerp(abc, bcd, t)
    return (c[0], ab, abc, mid), (mid, bcd, cd, c[3])


def _cubic_range(c, t0, t1):
    if t0 > 0.0:
        c = _cubic_split(c, t0)[1]
        t1 = (t1 - t0)/(1.0 - t0) if t0 < 1.0 else 1.0
    if t1 < 1.0:
        c = _cubic_split(c, t1)[0]
    return c


def _extrema(c):
    roots = []
    for axis in (0, 1):
        p0, p1, p2, p3 = (p[axis] for p in c)
        a = -p0 + 3*p1 - 3*p2 + p3
        b = 2*(p0 - 2*p1 + p2)
        d = p1 - p0
        if abs(a) < 1e-12:
            if abs(b) > 1e-12:
                roots.append(-d/b)
        else:
            disc = b*b - 4*a*d
            if disc >= 0:
                root = math.sqrt(disc)
                roots += [(-b + root)/(2*a), (-b - root)/(2*a)]
    return sorted(t for t in roots if 0.01 < t < 0.99)


class _Piece:
    def __init__(self, points):
        self.points = tuple(points)     # 2 for a line, 4 for a cubic
        self.t0, self.t1 = 0.0, 1.0
        self.offset = (0.0, 0.0)
        self.axis = None                # 0: vertical line (constant x), 1: horizontal line
        self.sign = (0, 0)
        self.limit = 0.0

    @property
    def start(self):
        return self.points[0]

    @property
    def end(self):
        return self.points[-1]

    def point(self, t):
        base = _lerp(self.points[0], self.points[1], t) if len(self.points) == 2 else _cubic_point(self.points, t)
        return (base[0] + self.offset[0], base[1] + self.offset[1])

    def tangent(self, at_end):
        pts = self.points[::-1] if at_end else self.points
        for other in pts[1:]:
            dx, dy = other[0] - pts[0][0], other[1] - pts[0][1]
            if abs(dx) + abs(dy) > 1e-7:
                length = math.hypot(dx, dy)
                return (-dx/length, -dy/length) if at_end else (dx/length, dy/length)
        return (0.0, 0.0)

    def crossing(self, axis, value):
        a, b = self.point(0.0)[axis] - value, self.point(1.0)[axis] - value
        if a*b > 0:
            return None
        lo, hi = 0.0, 1.0
        for _ in range(40):
            mid = (lo + hi)*0.5
            if (self.point(mid)[axis] - value)*a > 0:
                lo = mid
            else:
                hi = mid
        return (lo + hi)*0.5


def _flatten(pieces, steps=12):
    out = []
    for piece in pieces:
        if len(piece.points) == 2:
            out.append(piece.points[0])
        else:
            out.extend(_cubic_point(piece.points, i/steps) for i in range(steps))
    return out


def _winding(polygons, point):
    total, crossings = 0, 0
    x, y = point
    for poly in polygons:
        for i in range(len(poly)):
            a, b = poly[i], poly[(i + 1) % len(poly)]
            if (a[1] <= y) != (b[1] <= y):
                if a[0] + (y - a[1])*(b[0] - a[0])/(b[1] - a[1]) > x:
                    total += 1 if b[1] > a[1] else -1
                    crossings += 1
    return total, crossings


def _subpaths(cmds):
    paths, current, start, pos = [], [], None, None
    for c in cmds:
        if c[0] == 'M':
            if current:
                paths.append((current, start, pos))
            current, start, pos = [], (c[1], c[2]), (c[1], c[2])
        elif c[0] == 'L':
            current.append(((pos, (c[1], c[2]))))
            pos = (c[1], c[2])
        elif c[0] == 'C':
            current.append((pos, (c[1], c[2]), (c[3], c[4]), (c[5], c[6])))
            pos = (c[5], c[6])
        elif c[0] == 'Z' and current:
            paths.append((current, start, pos))
            current, pos = [], start
    if current:
        paths.append((current, start, pos))
    result = []
    for segments, first, last in paths:
        if math.hypot(first[0] - last[0], first[1] - last[1]) > 1e-6:
            segments = segments + [(last, first)]
        pieces = []
        for seg in segments:
            if math.hypot(seg[-1][0] - seg[0][0], seg[-1][1] - seg[0][1]) < 1e-6 and len(seg) == 2:
                continue
            if len(seg) == 4:
                straight = all(abs((p[0] - seg[0][0])*(seg[3][1] - seg[0][1]) -
                                   (p[1] - seg[0][1])*(seg[3][0] - seg[0][0])) < 1e-6 for p in seg[1:3])
                if straight:
                    pieces.append(_Piece((seg[0], seg[3])))
                    continue
                rest, last_t = seg, 0.0
                for t in _extrema(seg):
                    head, rest = _cubic_split(rest, (t - last_t)/(1.0 - last_t))
                    pieces.append(_Piece(head))
                    last_t = t
                pieces.append(_Piece(rest))
            else:
                pieces.append(_Piece(seg))
        if len(pieces) >= 2:
            result.append(pieces)
    return result


def _snap_distance(value, sign):
    scaled = value/GRID
    nearest = round(scaled)
    if abs(scaled - nearest) < GRID_EPS/GRID:
        return 0.0
    target = math.ceil(scaled) if sign > 0 else math.floor(scaled)
    return abs(target*GRID - value)


def _side(pieces, polygons, rule):
    votes = 0
    for piece in pieces:
        mid = piece.point(0.5)
        a, b = piece.point(0.45), piece.point(0.55)
        tx, ty = b[0] - a[0], b[1] - a[1]
        length = math.hypot(tx, ty)
        if length < 1e-9:
            continue
        nx, ny = ty/length*0.02, -tx/length*0.02
        inside = []
        for p in ((mid[0] + nx, mid[1] + ny), (mid[0] - nx, mid[1] - ny)):
            total, crossings = _winding(polygons, p)
            inside.append(crossings % 2 == 1 if rule == 'evenodd' else total != 0)
        if inside[0] != inside[1]:
            votes += -1 if inside[0] else 1
    return (votes > 0) - (votes < 0)


def _classify(piece, side, amount):
    dx, dy = piece.end[0] - piece.start[0], piece.end[1] - piece.start[1]
    nx, ny = side*dy, -side*dx
    if len(piece.points) == 2 and abs(dx) < AXIS_EPS:
        piece.axis, piece.sign = 0, (1 if nx > 0 else -1, 0)
        piece.limit = piece.start[0] + piece.sign[0]*_snap_distance(piece.start[0], piece.sign[0])
    elif len(piece.points) == 2 and abs(dy) < AXIS_EPS:
        piece.axis, piece.sign = 1, (0, 1 if ny > 0 else -1)
        piece.limit = piece.start[1] + piece.sign[1]*_snap_distance(piece.start[1], piece.sign[1])
    else:
        piece.sign = (1 if nx > 0 else -1, 1 if ny > 0 else -1)
        piece.offset = (piece.sign[0]*amount, piece.sign[1]*amount)
    piece.angle = math.degrees(math.atan2(piece.sign[1], piece.sign[0]))


def _on_axis(piece, point):
    return (piece.limit, point[1]) if piece.axis == 0 else (point[0], piece.limit)


def _intersect(a, b, steps=24):
    """First crossing of the end of piece a with the start of piece b, as (ta, tb)."""
    if len(a.points) == 2 and len(b.points) == 2:
        p, q = a.point(0.0), b.point(0.0)
        r = (a.point(1.0)[0] - p[0], a.point(1.0)[1] - p[1])
        s = (b.point(1.0)[0] - q[0], b.point(1.0)[1] - q[1])
        den = r[0]*s[1] - r[1]*s[0]
        if abs(den) < 1e-9:
            return None
        ta = ((q[0] - p[0])*s[1] - (q[1] - p[1])*s[0])/den
        tb = ((q[0] - p[0])*r[1] - (q[1] - p[1])*r[0])/den
        return (ta, tb) if 0.0 <= ta <= 1.0 and 0.0 <= tb <= 1.0 else None
    pa = [a.point(i/steps) for i in range(steps + 1)]
    pb = [b.point(i/steps) for i in range(steps + 1)]
    for i in range(steps - 1, -1, -1):
        for j in range(steps):
            p, q = pa[i], pb[j]
            r = (pa[i + 1][0] - p[0], pa[i + 1][1] - p[1])
            s = (pb[j + 1][0] - q[0], pb[j + 1][1] - q[1])
            den = r[0]*s[1] - r[1]*s[0]
            if abs(den) < 1e-12:
                continue
            u = ((q[0] - p[0])*s[1] - (q[1] - p[1])*s[0])/den
            v = ((q[0] - p[0])*r[1] - (q[1] - p[1])*r[0])/den
            if 0.0 <= u <= 1.0 and 0.0 <= v <= 1.0:
                return ((i + u)/steps, (j + v)/steps)
    return None


def _junction(a, b, side, amount):
    """Trims the pieces around their common vertex and returns the points that connect them."""
    vertex = a.end
    ta, tb = a.tangent(True), b.tangent(False)
    cross = ta[0]*tb[1] - ta[1]*tb[0]
    delta = (b.angle - a.angle + 180.0) % 360.0 - 180.0
    if abs(cross) > 1e-3:
        turn = 1 if cross > 0 else -1
    elif abs(delta) > 1e-6 and abs(delta) < 180.0 - 1e-6:
        turn = 1 if delta > 0 else -1
    else:
        turn = 0
    sweep = ((b.angle - a.angle)*turn) % 360.0 if turn else 0.0
    if sweep > 180.0 + 1e-6:
        sweep = 0.0

    if turn*side >= 0:
        limits = []
        steps = int(round(sweep/45.0))
        for i in range(steps + 1):
            angle = a.angle + turn*i*45.0
            if abs(angle/90.0 - round(angle/90.0)) > 1e-6:
                continue
            quarter = int(round(angle/90.0)) % 4
            axis, sign = (0, 1, 0, 1)[quarter], (1, 1, -1, -1)[quarter]
            if a.axis == axis and a.sign[axis] == sign and i == 0:
                value = a.limit
            elif b.axis == axis and b.sign[axis] == sign and i == steps:
                value = b.limit
            else:
                value = vertex[axis] + sign*min(_snap_distance(vertex[axis], sign), amount)
            limits.append((axis, sign, value))

        points = []
        if a.axis is None:
            if limits:
                axis, sign, value = limits[0]
                if (a.point(1.0)[axis] - value)*sign > EPS:
                    t = a.crossing(axis, value)
                    if t is not None:
                        a.t1 = t
            points.append(a.point(a.t1))
        for first, second in zip(limits, limits[1:]):
            corner = [0.0, 0.0]
            corner[first[0]], corner[second[0]] = first[2], second[2]
            points.append(tuple(corner))
        if b.axis is None:
            if limits:
                axis, sign, value = limits[-1]
                if (b.point(0.0)[axis] - value)*sign > EPS:
                    t = b.crossing(axis, value)
                    if t is not None:
                        b.t0 = t
            points.append(b.point(b.t0))
    else:
        points = []
        if a.axis is not None and b.axis is not None:
            if a.axis != b.axis:
                corner = [0.0, 0.0]
                corner[a.axis], corner[b.axis] = a.limit, b.limit
                points.append(tuple(corner))
        elif a.axis is not None or b.axis is not None:
            line, other = (a, b) if a.axis is not None else (b, a)
            t = other.crossing(line.axis, line.limit)
            if t is not None:
                if other is b:
                    b.t0 = t
                else:
                    a.t1 = t
            points.append(other.point(b.t0 if other is b else a.t1))
        else:
            hit = _intersect(a, b)
            if hit is not None:
                a.t1, b.t0 = hit
                points.append(a.point(a.t1))
            else:
                points += [a.point(1.0), b.point(0.0)]

    if points:
        if a.axis is not None:
            first = _on_axis(a, points[0])
            if math.hypot(first[0] - points[0][0], first[1] - points[0][1]) > 1e-9:
                points.insert(0, first)
        if b.axis is not None:
            last = _on_axis(b, points[-1])
            if math.hypot(last[0] - points[-1][0], last[1] - points[-1][1]) > 1e-9:
                points.append(last)
    return points


def _emit(pieces, joints):
    ops = []
    for index, piece in enumerate(pieces):
        if piece.axis is None and piece.t1 - piece.t0 > 1e-6:
            if len(piece.points) == 2:
                ops.append(('L', piece.point(piece.t0)))
                ops.append(('L', piece.point(piece.t1)))
            else:
                c = _cubic_range(piece.points, piece.t0, piece.t1)
                c = [(p[0] + piece.offset[0], p[1] + piece.offset[1]) for p in c]
                ops.append(('L', c[0]))
                ops.append(('C', c[1], c[2], c[3]))
        ops.extend(('L', p) for p in joints[index])

    def position(op):
        return op[-1]

    out = []
    for op in ops:
        if op[0] == 'L' and out and math.hypot(position(out[-1])[0] - op[1][0],
                                                position(out[-1])[1] - op[1][1]) < 5e-4:
            continue
        out.append(op)
    while len(out) > 1 and out[-1][0] == 'L' and math.hypot(position(out[-1])[0] - position(out[0])[0],
                                                            position(out[-1])[1] - position(out[0])[1]) < 5e-4 \
            and out[0][0] == 'L':
        out.pop()

    changed = True
    while changed and len(out) > 2:
        changed = False
        for i in range(len(out)):
            prev, cur, nxt = out[i - 1], out[i], out[(i + 1) % len(out)]
            if cur[0] != 'L' or nxt[0] != 'L':
                continue
            p, q, r = position(prev), cur[1], nxt[1]
            area = (q[0] - p[0])*(r[1] - p[1]) - (q[1] - p[1])*(r[0] - p[0])
            dot = (q[0] - p[0])*(r[0] - q[0]) + (q[1] - p[1])*(r[1] - q[1])
            if abs(area) < 1e-6 and dot >= 0:
                del out[i]
                changed = True
                break

    if len(out) < 2:
        return []
    start = out[0]
    if start[0] == 'C':
        cmds = [('M',) + position(out[-1])]
        body = out
        if out[-1][0] == 'L':
            body = out[:-1]
            cmds = [('M',) + out[-1][1]]
    else:
        cmds = [('M',) + start[1]]
        body = out[1:]
    for op in body:
        if op[0] == 'L':
            cmds.append(('L',) + op[1])
        else:
            cmds.append(('C',) + op[1] + op[2] + op[3])
    cmds.append(('Z',))
    return cmds


def expand_cmds(cmds, rule='nonzero', amount=0.125):
    """Returns the outline grown by the touch rule, or the same commands when nothing moves."""
    subpaths = _subpaths(cmds)
    if not subpaths:
        return cmds
    polygons = [_flatten(pieces) for pieces in subpaths]
    out, moved = [], False
    for pieces in subpaths:
        side = _side(pieces, polygons, rule)
        if side == 0:
            return cmds
        for piece in pieces:
            _classify(piece, side, amount)
        if any(piece.axis is None or abs(piece.limit - piece.start[piece.axis]) > 1e-9 for piece in pieces):
            moved = True
        joints = [_junction(pieces[i], pieces[(i + 1) % len(pieces)], side, amount) for i in range(len(pieces))]
        out.extend(_emit(pieces, joints))
    return out if moved and out else cmds


def expand_rect(rect):
    x, y, w, h, rx = rect
    if rx > 0:
        return rect
    x0, y0 = x - _snap_distance(x, -1), y - _snap_distance(y, -1)
    x1, y1 = x + w + _snap_distance(x + w, 1), y + h + _snap_distance(y + h, 1)
    return (x0, y0, x1 - x0, y1 - y0, rx)


def apply(shapes, amount=0.125):
    """Grows every fill without a stroke; returns the number of changed shapes."""
    changed = 0
    for shape in shapes:
        if shape.fill is None or shape.stroke is not None:
            continue
        if shape.rect is not None:
            rect = expand_rect(shape.rect)
            if rect != shape.rect:
                shape.rect = rect
                changed += 1
            continue
        cmds = expand_cmds(shape.cmds, shape.fill_rule, amount)
        if cmds is not shape.cmds:
            shape.cmds = cmds
            changed += 1
    return changed


def inset_cmds(cmds, distance, rule='nonzero'):
    """Moves an outline of axis-aligned lines and quarter arcs inwards; None when the path has other pieces.

    The edge that only closes an open path stays where it is.
    """
    explicit = sum(1 for c in cmds if c[0] in ('L', 'C'))
    subpaths = _subpaths(cmds)
    if len(subpaths) != 1:
        return None
    pieces = subpaths[0]
    side = _side(pieces, [_flatten(pieces)], rule)
    if side == 0:
        return None
    closing = None
    if not any(c[0] == 'Z' for c in cmds):
        first, last = pieces[0].start, pieces[-1]
        if len(last.points) == 2 and math.hypot(last.end[0] - first[0], last.end[1] - first[1]) < 1e-6:
            original_end = [c for c in cmds if c[0] in ('L', 'C')][-1][-2:]
            if math.hypot(original_end[0] - first[0], original_end[1] - first[1]) > 1e-6:
                closing = last
    out = []
    for piece in pieces:
        if piece is closing:
            continue
        ta, tb = piece.tangent(False), piece.tangent(True)
        normal = (side*ta[1], -side*ta[0])
        if len(piece.points) == 2:
            if min(abs(ta[0]), abs(ta[1])) > AXIS_EPS:
                return None
            moved = [(p[0] - normal[0]*distance, p[1] - normal[1]*distance) for p in piece.points]
        else:
            if min(abs(ta[0]), abs(ta[1])) > 1e-3 or min(abs(tb[0]), abs(tb[1])) > 1e-3 or \
                    abs(ta[0]*tb[0] + ta[1]*tb[1]) > 1e-3:
                return None
            start, end = piece.start, piece.end
            centre = (start[0], end[1]) if abs(ta[0]) > 0.5 else (end[0], start[1])
            radial = (start[0] - centre[0], start[1] - centre[1])
            radius = math.hypot(radial[0], radial[1])
            if radius < 1e-6:
                return None
            convex = radial[0]*normal[0] + radial[1]*normal[1] > 0
            factor = max(radius - distance, 0.0)/radius if convex else (radius + distance)/radius
            moved = [(centre[0] + (p[0] - centre[0])*factor, centre[1] + (p[1] - centre[1])*factor)
                     for p in piece.points]
        out.append(moved)
    if not out or explicit == 0:
        return None
    result = [('M',) + out[0][0]]
    position = out[0][0]
    for moved in out:
        if math.hypot(moved[0][0] - position[0], moved[0][1] - position[1]) > 1e-6:
            result.append(('L',) + moved[0])
        if len(moved) == 2:
            result.append(('L',) + moved[1])
        elif math.hypot(moved[3][0] - moved[0][0], moved[3][1] - moved[0][1]) > 1e-6:
            result.append(('C',) + moved[1] + moved[2] + moved[3])
        position = moved[-1]
    result.append(('Z',))
    return result
