#!/usr/bin/env python3
"""Measures how close converted SVGs are to the editor PNGs; writes report.csv, SUMMARY.md and contact sheets.

    python3 compare_svg.py --svg-dir <dir> --png-dir o2/Editor/Assets/ui [--used referenced_images.txt] [--sheets 40]
"""

import argparse
import csv
import json
import os

import numpy as np
from PIL import Image

import raw_svg
import render

TOL = (8, 24)


def load_used(path):
    used = set()
    if not path:
        return used
    with open(path, encoding='utf-8') as f:
        for line in f:
            cols = line.rstrip('\n').split('\t')
            if len(cols) >= 5 and cols[4] in ('CODE', 'DYNAMIC'):
                used.add(os.path.splitext(os.path.basename(cols[0]))[0])
    return used


def measure(svg_text, ref):
    """Similarity under a plain box-filter rasteriser and under the emulated Illustrator export rule."""
    page = raw_svg.RawPage(svg_text)
    plain = render.render_svg(svg_text)
    emulated = render.render_items_rule(page.w, page.h, page.items, 'ai')
    grown = render.render_items_rule(page.w, page.h, page.items, 'overscan')
    return plain, emulated, render.compare(plain, ref, TOL), render.compare(emulated, ref, TOL), render.compare(grown, ref, TOL)


def diagnose(row):
    if row['ai_tol24'] >= 0.97:
        return 'edge anti-aliasing only: matches under the Illustrator export rule'
    return row['notes'].split(' | ')[0]


def tile(img, k):
    rgb = np.clip(render.on_background(img, render.BACKGROUNDS['grey']), 0, 255).astype(np.uint8)
    return np.kron(rgb, np.ones((k, k, 1), dtype=np.uint8))


def contact_sheet(rows, images, path, k=4):
    """PNG | SVG render | diff x4 | render under the AI rule | its diff x4, one row per sprite."""
    strips = []
    for row in rows:
        ref, plain, emulated = images[row['name']]
        gap = np.full((ref.shape[0] * k, 4, 3), 30, dtype=np.uint8)
        cells = [tile(ref, k)]
        for img in (plain, emulated):
            d = np.clip(render.diff_map(img, ref) * 4, 0, 255).astype(np.uint8)
            cells += [gap, tile(img, k), gap, np.kron(np.stack([d] * 3, axis=2), np.ones((k, k, 1), dtype=np.uint8))]
        strips.append(np.concatenate(cells, axis=1))
    width = max(s.shape[1] for s in strips)
    sheet = [np.pad(s, ((0, 6), (0, width - s.shape[1]), (0, 0)), constant_values=30) for s in strips]
    Image.fromarray(np.concatenate(sheet, axis=0)).save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--svg-dir', required=True)
    ap.add_argument('--png-dir', required=True)
    ap.add_argument('--used', default=None, help='referenced_images.txt, marks sprites the editor code uses')
    ap.add_argument('--sheets', type=int, default=40, help='how many of the worst sprites get contact sheets')
    ap.add_argument('--sheet-key', default='tol24', choices=['tol24', 'ai_tol24', 'ov_tol24'],
                    help='measurement that orders the contact sheets')
    args = ap.parse_args()

    manifest = {}
    manifest_path = os.path.join(args.svg_dir, 'manifest.json')
    if os.path.exists(manifest_path):
        with open(manifest_path, encoding='utf-8') as f:
            manifest = json.load(f)
    used = load_used(args.used)
    rows, images = [], {}
    for fname in sorted(os.listdir(args.svg_dir)):
        if not fname.endswith('.svg'):
            continue
        name = fname[:-4]
        png = os.path.join(args.png_dir, name + '.png')
        if not os.path.exists(png):
            continue
        with open(os.path.join(args.svg_dir, fname), encoding='utf-8') as f:
            text = f.read()
        ref = render.load_png(png)
        plain, emulated, (sims, mad), (ai_sims, ai_mad), (ov_sims, ov_mad) = measure(text, ref)
        meta = dict(manifest.get(name, {}))
        if sims[1] < 0.97 and ai_sims[1] < 0.97 and meta.get('class') != 'flagged':
            meta['class'] = 'flagged'
            meta['notes'] = ['artwork differs from the PNG beyond edge anti-aliasing'] + meta.get('notes', [])
        images[name] = (ref, plain, emulated)
        rows.append({'name': name, 'page': meta.get('page', ''), 'class': meta.get('class', ''),
                     'primitives': meta.get('primitives', ''), 'tol8': sims[0], 'tol24': sims[1], 'mad': mad,
                     'ai_tol8': ai_sims[0], 'ai_tol24': ai_sims[1], 'ai_mad': ai_mad,
                     'ov_tol8': ov_sims[0], 'ov_tol24': ov_sims[1], 'ov_mad': ov_mad,
                     'pixels': ref.shape[0] * ref.shape[1],
                     'used': int(name in used), 'notes': ' | '.join(meta.get('notes', []))})

    with open(os.path.join(args.svg_dir, 'report.csv'), 'w', newline='', encoding='utf-8') as f:
        wr = csv.writer(f)
        wr.writerow(['name', 'page', 'class', 'primitives', 'similarity_tol8', 'similarity_tol24', 'mean_abs_diff',
                     'overscan_tol8', 'overscan_tol24', 'overscan_mean_abs_diff',
                     'ai_rule_tol8', 'ai_rule_tol24', 'ai_rule_mean_abs_diff', 'used_in_code', 'notes'])
        for r in rows:
            wr.writerow([r['name'], r['page'], r['class'], r['primitives'], '%.4f' % r['tol8'], '%.4f' % r['tol24'],
                         '%.3f' % r['mad'], '%.4f' % r['ov_tol8'], '%.4f' % r['ov_tol24'], '%.3f' % r['ov_mad'],
                         '%.4f' % r['ai_tol8'], '%.4f' % r['ai_tol24'], '%.3f' % r['ai_mad'],
                         r['used'], r['notes']])

    worst = sorted(rows, key=lambda r: (r[args.sheet_key], r['tol24']))[:args.sheets]
    sheet_files = []
    for i in range(0, len(worst), 10):
        path = os.path.join(args.svg_dir, 'sheet_worst_%02d.png' % (i // 10 + 1))
        contact_sheet(worst[i:i + 10], images, path)
        sheet_files.append((os.path.basename(path), [r['name'] for r in worst[i:i + 10]]))

    def stats(sel, key):
        v = np.array([r[key] for r in sel])
        w = np.array([r['pixels'] for r in sel], dtype=float)
        return v.mean(), float((v * w).sum() / w.sum()), int((v >= 0.99).sum()), int((v >= 0.97).sum())

    lines = ['# Vector sprites vs editor PNG', '',
             'Similarity = share of pixels whose max channel difference is within the tolerance, worst of three',
             'backgrounds (#606060, white, black). "plain" is a box-filter coverage rasteriser (cairo); "overscan" is the',
             'same with every fill grown by 0.125 px; "AI rule" emulates the Illustrator export exactly (4x4 sub-pixels,',
             'a sub-pixel is on when a fill touches it, strokes 0.25 px thinner).', '',
             '| set | sprites | renderer | mean tol8 | mean tol24 | pixel-weighted tol24 | >=99% tol24 | >=97% tol24 |',
             '|---|---|---|---|---|---|---|---|']
    for label, sel in (('all', rows), ('used by the editor', [r for r in rows if r['used']])):
        if not sel:
            continue
        for rname, k8, k24 in (('plain', 'tol8', 'tol24'), ('overscan', 'ov_tol8', 'ov_tol24'),
                               ('AI rule', 'ai_tol8', 'ai_tol24')):
            m24, w24, n99, n97 = stats(sel, k24)
            lines.append('| %s | %d | %s | %.2f%% | %.2f%% | %.2f%% | %d | %d |' % (
                label, len(sel), rname, stats(sel, k8)[0] * 100, m24 * 100, w24 * 100, n99, n97))
    lines += ['', '## Classes', '', '| class | sprites | mean tol8 | mean tol24 |', '|---|---|---|---|']
    for cls in sorted(set(r['class'] for r in rows)):
        sel = [r for r in rows if r['class'] == cls]
        lines.append('| %s | %d | %.2f%% | %.2f%% |' % (cls, len(sel), stats(sel, 'tol8')[0] * 100, stats(sel, 'tol24')[0] * 100))
    lines += ['', '## Distribution (plain renderer, tol 24)', '', '| range | sprites |', '|---|---|']
    for lo, hi in ((0.99, 1.01), (0.97, 0.99), (0.95, 0.97), (0.90, 0.95), (0.0, 0.90)):
        lines.append('| %d%%..%s | %d |' % (lo * 100, '100%' if hi > 1 else '%d%%' % (hi * 100),
                                             sum(1 for r in rows if lo <= r['tol24'] < hi)))
    below = sorted((r for r in rows if r['tol24'] < 0.97), key=lambda r: (-r['used'], r['tol24']))
    lines += ['', '## Below 97%% at tol 24 (%d), used first, worst first' % len(below), '',
              '| sprite | used | class | plain tol24 | AI rule tol24 | diagnosis |', '|---|---|---|---|---|---|']
    for r in below:
        lines.append('| %s | %s | %s | %.1f%% | %.1f%% | %s |' % (
            r['name'], 'yes' if r['used'] else 'no', r['class'], r['tol24'] * 100, r['ai_tol24'] * 100, diagnose(r)))
    flagged = [r for r in rows if r['class'] == 'flagged']
    lines += ['', '## Flagged (%d)' % len(flagged), '']
    for r in flagged:
        lines.append('- %s: %s' % (r['name'], r['notes']))
    lines += ['', '## Contact sheets (PNG | plain render | diff x4 | AI-rule render | diff x4; upscaled x4), worst first', '']
    for fname, names in sheet_files:
        lines.append('- %s: %s' % (fname, ', '.join(names)))
    with open(os.path.join(args.svg_dir, 'SUMMARY.md'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print('\n'.join(lines[:14]))


if __name__ == '__main__':
    main()
