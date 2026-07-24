#!/usr/bin/env python3
"""Core PSD helpers: layer tree, composite rendering, per-layer extraction.

Built on psd-tools (pip install psd-tools). Coordinates in the tree are PSD
pixels (origin top-left, y down); o2-space conversion lives in psd_to_o2.py.
"""

import os
import re

from psd_tools import PSDImage


def load_psd(path):
    if not os.path.isfile(path):
        raise FileNotFoundError(f"PSD not found: {path}")
    return PSDImage.open(path)


def layer_kind(layer):
    if layer.is_group():
        return "group"
    return layer.kind  # pixel, type, shape, smartobject, adjustment, ...


def layer_name(layer):
    # some writers null-terminate unicode layer names
    return layer.name.rstrip("\x00")


def layer_node(layer, include_hidden):
    """One tree node; returns None for skipped (hidden) layers."""
    if not include_hidden and not layer.visible:
        return None

    bbox = layer.bbox  # (left, top, right, bottom), (0,0,0,0) when empty
    node = {
        "name": layer_name(layer),
        "kind": layer_kind(layer),
        "visible": bool(layer.visible),
        "opacity": int(layer.opacity),
        "blend_mode": str(layer.blend_mode).split(".")[-1].lower(),
        "bbox": [int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])],
        "size": [int(bbox[2] - bbox[0]), int(bbox[3] - bbox[1])],
    }

    if layer.is_group():
        children = [layer_node(child, include_hidden) for child in layer]
        node["children"] = [child for child in children if child is not None]

    return node


def structure(psd_path, include_hidden=False):
    """Full document tree: canvas size + layers bottom-to-top."""
    psd = load_psd(psd_path)
    layers = [layer_node(layer, include_hidden) for layer in psd]
    return {
        "canvas": [psd.width, psd.height],
        "layers": [layer for layer in layers if layer is not None],
    }


def flat_layers(psd_path, include_hidden=False, groups=False):
    """Depth-first flat list of layers with slash paths, bottom-to-top."""
    psd = load_psd(psd_path)
    result = []

    def walk(layer, prefix):
        if not include_hidden and not layer.visible:
            return
        path = prefix + layer_name(layer)
        if layer.is_group():
            if groups:
                result.append((path, layer))
            for child in layer:
                walk(child, path + "/")
        else:
            result.append((path, layer))

    for layer in psd:
        walk(layer, "")
    return psd, result


def render(psd_path, out_path, scale=None, prefer_preview=True):
    """Renders the document into a PNG; returns the PIL image.

    Photoshop files carry a flattened preview - using it is instant, cheap on
    memory and pixel-exact to what the artist saw. Files without a preview
    (or with prefer_preview=False) are composited from layers.
    """
    psd = load_psd(psd_path)
    image = None
    if prefer_preview and psd.has_preview():
        image = psd.topil()
    if image is None:
        image = psd.composite(force=True)
    if image is None:
        raise ValueError("PSD has no composable content")
    if scale and scale != 1.0:
        image = image.resize((max(1, round(image.width * scale)),
                              max(1, round(image.height * scale))))
    save_png(image, out_path)
    return image


def safe_file_name(name, taken):
    base = re.sub(r"[^\w\- ]+", "_", name).strip().replace(" ", "_") or "layer"
    candidate, index = base, 1
    while candidate.lower() in taken:
        index += 1
        candidate = f"{base}_{index}"
    taken.add(candidate.lower())
    return candidate


def extract_layers(psd_path, out_dir, layer_names=None, include_hidden=False):
    """Saves every raster layer as an RGBA PNG named after the layer.

    layer_names filters by exact layer name or slash path. Returns list of
    dicts: name, path (slash path), file, bbox.
    """
    psd, layers = flat_layers(psd_path, include_hidden)
    os.makedirs(out_dir, exist_ok=True)

    wanted = set(layer_names) if layer_names else None
    taken = set()
    saved = []
    for path, layer in layers:
        if wanted is not None and path not in wanted and layer_name(layer) not in wanted:
            continue

        image = layer.composite()
        if image is None or layer.width == 0 or layer.height == 0:
            continue

        file_path = os.path.join(out_dir, safe_file_name(layer_name(layer), taken) + ".png")
        save_png(image, file_path)
        bbox = layer.bbox
        saved.append({
            "name": layer_name(layer),
            "path": path,
            "file": file_path,
            "bbox": [int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])],
        })

    return saved


def save_png(image, out_path):
    out_dir = os.path.dirname(os.path.abspath(out_path))
    os.makedirs(out_dir, exist_ok=True)
    image.save(out_path, format="PNG")
    return out_path
