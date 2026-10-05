"""Rebuilds rasterised soft shadows (Illustrator effects) from a handful of vector primitives.

Model: the shadow alpha depends only on the distance to an axis-aligned core rectangle. It is drawn as
the core rectangle, four side strips with a linear alpha ramp and four corner squares with a radial one.
A core that collapses to a line or a point gives the shadow of a pill or of a circle.
"""

import numpy as np

from svg_model import Gradient, Shape, rect_cmds

BIN = 0.25


def _distance(shape_hw, core):
    h, w = shape_hw
    px = np.arange(w) + 0.5
    py = np.arange(h) + 0.5
    dx = np.maximum(np.maximum(core[0] - px, px - core[2]), 0.0)
    dy = np.maximum(np.maximum(core[1] - py, py - core[3]), 0.0)
    return np.hypot(dx[None, :], dy[:, None])


def _profile(target, weight, core):
    d = _distance(target.shape, core)
    idx = np.minimum((d / BIN).astype(int), 4 * 256)
    idx[d > 0] += 1
    n = int(idx.max()) + 1
    wsum = np.bincount(idx.ravel(), weight.ravel(), n)
    prof = np.bincount(idx.ravel(), (weight * target).ravel(), n) / np.maximum(wsum, 1e-12)
    err = float(np.sqrt((weight * (target - prof[idx]) ** 2).sum() / weight.sum()))
    return err, prof, wsum, idx


def _search_core(target, weight):
    h, w = target.shape
    peak = target.max()
    ys, xs = np.nonzero(target > 0.5 * peak)
    box = [int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1]
    far = max(w, h)
    free = [box[0] > 0, box[1] > 0, box[2] < w, box[3] < h]
    box = [box[0] if free[0] else -far, box[1] if free[1] else -far,
           box[2] if free[2] else w + far, box[3] if free[3] else h + far]
    best = None
    for k in range(0, max(w, h)):
        core = [box[0] + k * free[0], box[1] + k * free[1], box[2] - k * free[2], box[3] - k * free[3]]
        if core[0] > core[2]:
            core[0] = core[2] = (core[0] + core[2]) // 2
        if core[1] > core[3]:
            core[1] = core[3] = (core[1] + core[3]) // 2
        err = _profile(target, weight, core)[0]
        if best is None or err < best[0] - 1e-9:
            best = (err, core)
        if core[0] == core[2] and core[1] == core[3]:
            break
    err, core = best
    improved = True
    while improved:
        improved = False
        for side in range(4):
            if not free[side]:
                continue
            for step in (-1, 1):
                cand = list(core)
                cand[side] += step
                if cand[0] > cand[2] or cand[1] > cand[3]:
                    continue
                e = _profile(target, weight, cand)[0]
                if e < err - 1e-9:
                    err, core, improved = e, cand, True
    return core


def _ramp(prof, wsum):
    """Piecewise-linear alpha(distance) with few knots."""
    ds, vals = [0.0], [prof[0] if wsum[0] > 0 else prof[np.nonzero(wsum)[0][0]]]
    for i in range(1, len(prof)):
        if wsum[i] > 0:
            ds.append((i - 0.5) * BIN)
            vals.append(prof[i])
    ds, vals = np.array(ds), np.array(vals)
    vals = np.minimum.accumulate(np.maximum(vals, 0.0))
    visible = np.nonzero(vals > 0.4 / 255)[0]
    end = float(np.ceil(ds[min(visible[-1] + 1, len(ds) - 1)] * 2) / 2) if len(visible) else 1.0
    keep = ds < end
    ds, vals = np.append(ds[keep], end), np.append(vals[keep], 0.0)
    knots = [0, len(ds) - 1]

    def split(i0, i1, tol):
        line = np.interp(ds[i0:i1 + 1], [ds[i0], ds[i1]], [vals[i0], vals[i1]])
        dev = np.abs(line - vals[i0:i1 + 1])
        k = int(dev.argmax())
        if dev[k] > tol:
            knots.append(i0 + k)
            split(i0, i0 + k, tol)
            split(i0 + k, i1, tol)

    tol = 0.6 / 255
    while True:
        del knots[2:]
        split(0, len(ds) - 1, tol)
        if len(knots) <= 7:
            break
        tol *= 1.5
    knots = sorted(set(knots))
    return [(float(ds[k]), float(vals[k])) for k in knots]


def _prim(w, h, x0, y0, x1, y1, paint):
    x0, y0, x1, y1 = max(x0, 0.0), max(y0, 0.0), min(x1, float(w)), min(y1, float(h))
    if x1 - x0 < 1e-6 or y1 - y0 < 1e-6:
        return None
    s = Shape(rect_cmds(x0, y0, x1, y1))
    s.rect = (x0, y0, x1 - x0, y1 - y0, 0.0)
    if isinstance(paint, float):
        s.fill = (0.0, 0.0, 0.0)
        s.fill_opacity = paint
    else:
        s.fill = paint
    return s


def fit_shadow(raster, visibility):
    """Returns (shapes, info) approximating a Raster layer; info carries the fit error and parameters."""
    target = raster.rgba[..., 3] / 255.0
    h, w = target.shape
    weight = 0.05 + visibility
    core = _search_core(target, weight)
    err, prof, wsum, idx = _profile(target, weight, core)
    knots = _ramp(prof, wsum)
    reach = knots[-1][0]
    color = tuple(float((raster.rgba[..., c] * target).sum() / max(target.sum(), 1e-9)) for c in range(3))
    stops = [(d / reach, color, a) for d, a in knots]
    x0, y0, x1, y1 = [float(v) for v in core]
    shapes = []

    def add(ax0, ay0, ax1, ay1, paint):
        s = _prim(w, h, ax0, ay0, ax1, ay1, paint)
        if s is not None:
            if isinstance(paint, float):
                s.fill = color
            shapes.append(s)

    add(x0, y0, x1, y1, knots[0][1])
    add(x0, y0 - reach, x1, y0, Gradient('linear', (0, y0, 0, y0 - reach), stops))
    add(x0, y1, x1, y1 + reach, Gradient('linear', (0, y1, 0, y1 + reach), stops))
    add(x0 - reach, y0, x0, y1, Gradient('linear', (x0, 0, x0 - reach, 0), stops))
    add(x1, y0, x1 + reach, y1, Gradient('linear', (x1, 0, x1 + reach, 0), stops))
    xs = [(x0 - reach, x0, x0), (x1, x1 + reach, x1)] if x1 > x0 else [(x0 - reach, x0 + reach, x0)]
    ys = [(y0 - reach, y0, y0), (y1, y1 + reach, y1)] if y1 > y0 else [(y0 - reach, y0 + reach, y0)]
    for ax0, ax1, cx in xs:
        for ay0, ay1, cy in ys:
            add(ax0, ay0, ax1, ay1, Gradient('radial', (cx, cy, reach), stops))
    model = np.interp(_distance(target.shape, core), [k[0] for k in knots], [k[1] for k in knots])
    vis_err = np.abs(model - target) * visibility
    info = {'core': core, 'reach': reach, 'peak': knots[0][1], 'knots': knots,
            'max_err': float(vis_err.max() * 255), 'rms_err': err * 255}
    return shapes, info
