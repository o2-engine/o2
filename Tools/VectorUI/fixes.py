"""Per-sprite corrections for artboards that were edited after the editor PNG was exported."""

from svg_model import Gradient, Shape, rect_cmds

FIXES = {}


def fix(*names):
    def register(fn):
        for n in names:
            FIXES[n] = fn
        return fn
    return register


def apply(name, page, notes):
    fn = FIXES.get(name)
    if fn is not None:
        notes.append('manual fix: ' + fn(page))


@fix('UI4_shadow_separator')
def shadow_separator(page):
    # the PNG holds three rows of a white-to-slate ramp whose alpha is the ramp position, sampled half a pixel lower
    s = Shape(rect_cmds(1, 1, 49, 4))
    s.fill = Gradient('linear', (0, 1, 0, 4.5), [(0.0, (255, 255, 255), 0.0), (1.0, (126, 149, 160), 1.0)])
    page.items = [s]
    return 'rebuilt from the PNG: rows 1..3 only, ramp shifted by 0.5 px'
