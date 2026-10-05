#!/usr/bin/env python3
"""Copies fitted SVGs next to the PNG sprites of the editor and writes their .svg.meta files."""
import argparse
import hashlib
import json
import os
import re
import sys

META_TYPE = 'o2::VectorImageAsset::Meta'
ID_PATTERN = re.compile(r'"mId"\s*:\s*"([0-9a-fA-F]{32})"')


def collect(src):
    names = []
    for root, _, files in os.walk(src):
        for file in files:
            if file.endswith('.svg'):
                names.append(os.path.relpath(os.path.join(root, file), src)[:-4].replace(os.sep, '/'))
    return sorted(names)


def vector_id(name):
    return hashlib.md5(('vector:' + name + '.svg').encode('utf-8')).hexdigest()


def foreign_ids(assets_root, own_metas):
    ids = {}
    for root, _, files in os.walk(assets_root):
        for file in files:
            path = os.path.join(root, file)
            if not file.endswith('.meta') or os.path.abspath(path) in own_metas:
                continue
            with open(path, encoding='utf-8') as meta:
                match = ID_PATTERN.search(meta.read())
            if match:
                ids[match.group(1).lower()] = path
    return ids


def make_meta(name, png_meta_path):
    with open(png_meta_path, encoding='utf-8') as file:
        png_value = json.load(file)['Value']
    value = {'mId': vector_id(name)}
    for key in ('sliceBorder', 'defaultMode'):
        if key in png_value:
            value[key] = png_value[key]
    return json.dumps({'Type': META_TYPE, 'Value': value}, indent=4)


def write_if_changed(path, data):
    if os.path.isfile(path):
        with open(path, 'rb') as file:
            if file.read() == data:
                return False
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as file:
        file.write(data)
    return True


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--src', required=True, help='directory with the fitted SVGs, laid out like the ui folder')
    parser.add_argument('--dst', default=os.path.join(here, '..', '..', 'Editor', 'Assets', 'ui'),
                        help='ui folder of the editor assets')
    parser.add_argument('--assets-root', default=None, help='assets root the ids must be unique in (parent of --dst)')
    parser.add_argument('--dry-run', action='store_true', help='report without writing')
    args = parser.parse_args()

    dst = os.path.abspath(args.dst)
    assets_root = os.path.abspath(args.assets_root or os.path.join(dst, '..'))
    names = collect(args.src)

    imported = [name for name in names if os.path.isfile(os.path.join(dst, name + '.png'))
                and os.path.isfile(os.path.join(dst, name + '.png.meta'))]
    skipped = [name for name in names if name not in imported]

    own_metas = {os.path.abspath(os.path.join(dst, name + '.svg.meta')) for name in imported}
    used_ids = foreign_ids(assets_root, own_metas)
    seen = {}
    for name in imported:
        uid = vector_id(name)
        if uid in used_ids or uid in seen:
            sys.exit('id %s of %s.svg collides with %s' % (uid, name, used_ids.get(uid) or seen[uid]))
        seen[uid] = name + '.svg'

    written = 0
    for name in imported:
        with open(os.path.join(args.src, name + '.svg'), 'rb') as file:
            svg = file.read()
        meta = make_meta(name, os.path.join(dst, name + '.png.meta')).encode('utf-8')
        if args.dry_run:
            continue
        written += write_if_changed(os.path.join(dst, name + '.svg'), svg)
        written += write_if_changed(os.path.join(dst, name + '.svg.meta'), meta)

    for name in skipped:
        print('skipped, no png with the same name: ' + name, file=sys.stderr)
    print('svg: %d imported, %d skipped, %d files written' % (len(imported), len(skipped), written))


if __name__ == '__main__':
    main()
