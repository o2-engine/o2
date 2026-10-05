#!/usr/bin/env python3
"""Measures every SVG of a directory against its PNG original with the engine rasterizer (o2SvgRasterizer)."""
import argparse
import csv
import os
import re
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

BACKGROUNDS = ('606060', 'ffffff', '000000')
TOLERANCES = (8, 24)
PASS_SIMILARITY = 0.98
PASS_MEAN = 3.0
PRIMITIVE = re.compile(r'<(path|rect|circle|ellipse|line|polyline|polygon)\b')
SHEET_BACKGROUND = 128


def find_cli(explicit):
    path = explicit or os.environ.get('O2_SVG_RASTERIZER')
    if not path:
        here = os.path.dirname(os.path.abspath(__file__))
        for platform in ('Mac', 'Linux', 'Windows'):
            for name in ('o2SvgRasterizer', 'o2SvgRasterizer.exe'):
                candidate = os.path.join(here, '..', '..', '..', 'Bin', platform, name)
                if os.path.isfile(candidate):
                    path = candidate
    if not path or not os.path.isfile(path):
        sys.exit('o2SvgRasterizer not found: pass --cli or set O2_SVG_RASTERIZER')
    return os.path.abspath(path)


def collect(svg_dir, png_dir, only):
    names = []
    for root, _, files in os.walk(svg_dir):
        for file in files:
            if file.endswith('.svg'):
                name = os.path.relpath(os.path.join(root, file), svg_dir)[:-4].replace(os.sep, '/')
                names.append(name)
    names.sort()
    if only:
        wanted = set(only)
        missing = wanted - set(names)
        if missing:
            print('no svg for: ' + ', '.join(sorted(missing)), file=sys.stderr)
        names = [name for name in names if name in wanted]
    pairs = []
    for name in names:
        png = os.path.join(png_dir, name + '.png')
        if os.path.isfile(png):
            pairs.append((name, os.path.join(svg_dir, name + '.svg'), png))
        else:
            print('no png for: ' + name, file=sys.stderr)
    return pairs


def run_compare(cli, pairs, tolerance, background, work_dir, out_dir=None):
    list_path = os.path.join(work_dir, 'list_%d_%s_%s.txt' % (tolerance, background, os.path.basename(out_dir or '')))
    with open(list_path, 'w') as file:
        for _, svg, png in pairs:
            file.write('%s\t%s\n' % (os.path.abspath(svg), os.path.abspath(png)))
    command = [cli, '--compare', list_path, '--tolerance', str(tolerance), '--bg', background, '--quiet']
    if out_dir:
        command += ['--out-dir', out_dir]
    output = subprocess.run(command, capture_output=True, text=True).stdout
    result = {}
    rows = [line.split('\t') for line in output.splitlines() if line.count('\t') >= 3]
    for (name, _, _), row in zip(pairs, rows):
        result[name] = (float(row[0]), float(row[1]))
    return result


def composite(image, background):
    rgb = image[:, :, :3].astype(np.int32)
    alpha = image[:, :, 3:4].astype(np.int32)
    return (rgb*alpha + background*(255 - alpha) + 127)//255


def difference_map(reference, rendered):
    worst = np.zeros(reference.shape[:2], np.int32)
    for background in BACKGROUNDS:
        value = int(background[:2], 16)
        diff = np.abs(composite(reference, value) - composite(rendered, value)).max(axis=2)
        worst = np.maximum(worst, diff)
    return worst


def write_sheet(path, reference, rendered, diff):
    height, width = diff.shape
    zoom = 8 if max(width, height) <= 32 else 4
    gap = 4
    sheet = np.full((height, width*3 + gap*2, 3), SHEET_BACKGROUND, np.uint8)
    sheet[:, :width] = composite(reference, SHEET_BACKGROUND)
    sheet[:, width + gap:width*2 + gap] = composite(rendered, SHEET_BACKGROUND)
    amplified = np.clip(diff*8, 0, 255)
    more = rendered[:, :, 3].astype(np.int32) > reference[:, :, 3].astype(np.int32)
    less = rendered[:, :, 3].astype(np.int32) < reference[:, :, 3].astype(np.int32)
    panel = np.zeros((height, width, 3), np.int32)
    panel[:, :, 0] = np.where(less, amplified//3, amplified)
    panel[:, :, 1] = np.where(more | less, amplified//3, amplified)
    panel[:, :, 2] = np.where(more, amplified//3, amplified)
    sheet[:, width*2 + gap*2:] = panel
    image = Image.fromarray(sheet).resize((sheet.shape[1]*zoom, sheet.shape[0]*zoom), Image.NEAREST)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.save(path)


def measure(cli, pairs, out_dir, sheets):
    rows = {name: {'name': name, 'sim8': 1.0, 'sim24': 1.0, 'mean': 0.0, 'max': 0, 'bad24': 0}
            for name, _, _ in pairs}
    with tempfile.TemporaryDirectory() as work_dir:
        for tolerance in TOLERANCES:
            for background in BACKGROUNDS:
                result = run_compare(cli, pairs, tolerance, background, work_dir)
                for name, _, _ in pairs:
                    similarity, mean = result.get(name, (-1.0, -1.0))
                    row = rows[name]
                    key = 'sim%d' % tolerance
                    row[key] = min(row[key], similarity)
                    row['mean'] = -1.0 if mean < 0 else max(row['mean'], mean)

        groups = {}
        for pair in pairs:
            groups.setdefault(os.path.dirname(pair[0]), []).append(pair)
        for index, (group, group_pairs) in enumerate(sorted(groups.items())):
            render_dir = os.path.join(work_dir, 'render%d' % index)
            run_compare(cli, group_pairs, 24, BACKGROUNDS[0], work_dir, render_dir)
            for name, svg, png in group_pairs:
                row = rows[name]
                reference = np.asarray(Image.open(png).convert('RGBA'))
                row['width'], row['height'] = reference.shape[1], reference.shape[0]
                with open(svg) as file:
                    row['primitives'] = len(PRIMITIVE.findall(file.read()))
                render_path = os.path.join(render_dir, os.path.basename(name) + '.png')
                if not os.path.isfile(render_path):
                    row['sim8'] = row['sim24'] = row['mean'] = -1.0
                    continue
                rendered = np.asarray(Image.open(render_path).convert('RGBA'))
                diff = difference_map(reference, rendered)
                row['max'] = int(diff.max())
                row['bad24'] = int(round((1.0 - row['sim24'])*diff.size))
                if sheets:
                    write_sheet(os.path.join(out_dir, 'sheets', name + '.png'), reference, rendered, diff)

    for row in rows.values():
        row['pass'] = int(row['sim24'] >= PASS_SIMILARITY and 0 <= row['mean'] <= PASS_MEAN)
    return [rows[name] for name, _, _ in pairs]


def read_used(path):
    used = set()
    section = ''
    with open(path) as file:
        for line in file:
            if line.startswith('## '):
                section = line.split()[1]
            elif not line.startswith('#') and section in ('CODE', 'DYNAMIC') and line.strip():
                name = line.split('\t')[0]
                used.add(name[3:-4] if name.startswith('ui/') else name[:-4])
    return used


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--svg-dir', required=True)
    parser.add_argument('--png-dir', required=True)
    parser.add_argument('--only', help='comma separated names relative to the svg dir, without extension')
    parser.add_argument('--out', help='directory for report.csv and sheets (default: the svg dir)')
    parser.add_argument('--sheet', action='store_true', help='write sheets/<name>.png: PNG | engine | diff x8')
    parser.add_argument('--cli', help='path to o2SvgRasterizer (or env O2_SVG_RASTERIZER)')
    parser.add_argument('--used', help='referenced_images.txt: adds the used column and the used-only summary')
    parser.add_argument('--quiet', action='store_true', help='print the summary only')
    args = parser.parse_args()

    cli = find_cli(args.cli)
    only = [name.strip() for name in args.only.split(',') if name.strip()] if args.only else None
    pairs = collect(args.svg_dir, args.png_dir, only)
    if not pairs:
        sys.exit('nothing to check')
    out_dir = args.out or args.svg_dir
    os.makedirs(out_dir, exist_ok=True)

    rows = measure(cli, pairs, out_dir, args.sheet)
    used = read_used(args.used) if args.used else None
    fields = ['name', 'width', 'height', 'primitives', 'sim8', 'sim24', 'mean', 'max', 'bad24', 'pass']
    if used is not None:
        fields.append('used')
        for row in rows:
            row['used'] = int(row['name'] in used)

    with open(os.path.join(out_dir, 'report.csv'), 'w', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: ('%.5f' % row[key] if key in ('sim8', 'sim24') else
                                   '%.3f' % row[key] if key == 'mean' else row.get(key, '')) for key in fields})

    if not args.quiet:
        print('%-44s %7s %7s %6s %4s %5s %s' % ('name', 'sim8', 'sim24', 'mean', 'max', 'bad24', 'pass'))
        for row in sorted(rows, key=lambda row: (row['pass'], row['sim24'])):
            print('%-44s %7.4f %7.4f %6.2f %4d %5d %s' % (row['name'], row['sim8'], row['sim24'], row['mean'],
                                                         row['max'], row['bad24'], 'ok' if row['pass'] else 'FAIL'))

    def summary(title, subset):
        if subset:
            print('%s: %d sprites, mean sim8 %.4f, mean sim24 %.4f, pass %d, fail %d' % (
                title, len(subset), sum(row['sim8'] for row in subset)/len(subset),
                sum(row['sim24'] for row in subset)/len(subset),
                sum(row['pass'] for row in subset), sum(1 - row['pass'] for row in subset)))

    summary('all', rows)
    if used is not None:
        summary('used', [row for row in rows if row['used']])


if __name__ == '__main__':
    main()
