"""Artboard names and per-page raw SVG of an Illustrator .ai (PDF-compatible) file."""

import os
import re
import subprocess
import zlib

import pypdf


def artboard_names(ai_path):
    """One name per page, read from the Illustrator private data."""
    reader = pypdf.PdfReader(ai_path)
    private = reader.pages[0]['/PieceInfo']['/Illustrator']['/Private']
    keys = sorted((k for k in private.keys() if re.match(r'/AIPrivateData\d+$', k)), key=lambda k: int(k[14:]))
    blob = b''.join(private[k].get_data() for k in keys)
    marker = b'%AI12_CompressedData'
    at = blob.index(marker) + len(marker)
    text = zlib.decompressobj().decompress(blob[at:]).decode('latin-1')
    head = text[:text.index('(ArtboardArray)')]
    names = re.findall(r'\((.*?)\) /UnicodeString \(Name\) ,', head)[-len(reader.pages):]
    if len(names) != len(reader.pages):
        raise RuntimeError('found %d artboard names for %d pages' % (len(names), len(reader.pages)))
    return names


def png_name(page, artboard, seen):
    """Editor PNG base name for an artboard, None for artboards that are not sprites."""
    if page == 1 and artboard == 'Editor':
        return None
    if artboard == 'function_icon':
        return 'function_icon'
    name = 'UI4_' + artboard
    if name in seen:
        name = '%s-%d' % (name, page)
    return name


def page_svg(ai_path, page, cache_dir=None):
    if cache_dir:
        cached = os.path.join(cache_dir, 'p%d.svg' % page)
        if os.path.exists(cached) and os.path.getmtime(cached) >= os.path.getmtime(ai_path):
            with open(cached, encoding='utf-8') as f:
                return f.read()
    out = subprocess.run(['pdftocairo', '-svg', '-f', str(page), '-l', str(page), ai_path, '-'],
                         check=True, capture_output=True).stdout.decode('utf-8')
    if cache_dir:
        os.makedirs(cache_dir, exist_ok=True)
        with open(cached, 'w', encoding='utf-8') as f:
            f.write(out)
    return out
