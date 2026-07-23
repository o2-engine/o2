#!/usr/bin/env python3
"""Builds an o2 actor prefab (.proto) replicating a PSD layout.

Groups become container actors, raster layers become actors with an
ImageComponent; the full PSD hierarchy, layer order, positions, sizes and
opacities are preserved. Layer images are extracted into an assets folder next
to the prefab, with .meta files so the prefab references stable asset ids.

PSD pixels (origin top-left, y down) map to o2 world (origin at canvas center,
y up): the prefab root sits at (0, 0) and spans the canvas size.

CLI: psd_to_o2.py <psd> <out-dir-under-Assets> [--assets-root PATH]
     [--atlas Basic.atlas] [--scale 1.0] [--include-hidden]
"""

import argparse
import json
import os
import random
import uuid

import psd_lib


def new_uid():
    return uuid.uuid4().hex


def new_id():
    return random.getrandbits(63) | (1 << 62)


def vec(x, y):
    return {"x": round(float(x), 2), "y": round(float(y), 2)}


def read_asset_id(meta_path):
    try:
        with open(meta_path) as f:
            return json.load(f)["Value"]["mId"]
    except (OSError, KeyError, ValueError):
        return None


def write_meta(path, meta_type, value):
    with open(path, "w") as f:
        json.dump({"Type": meta_type, "Value": value}, f, indent=4)
        f.write("\n")


class PrefabBuilder:
    def __init__(self, assets_root, out_dir, atlas, scale, include_hidden):
        self.assets_root = os.path.abspath(assets_root)
        self.out_dir = out_dir.strip("/")
        self.atlas_id = None
        self.scale = scale
        self.include_hidden = include_hidden
        self.taken_names = set()
        self.images = []

        if atlas:
            self.atlas_id = read_asset_id(os.path.join(self.assets_root, atlas + ".meta"))

    def out_abs(self):
        return os.path.join(self.assets_root, self.out_dir)

    def save_layer_image(self, layer):
        image = layer.composite()
        if image is None:
            return None, None

        name = psd_lib.safe_file_name(psd_lib.layer_name(layer), self.taken_names)
        rel_path = self.out_dir + "/" + name + ".png"
        abs_path = os.path.join(self.assets_root, rel_path)
        psd_lib.save_png(image, abs_path)

        image_id = new_uid()
        meta_value = {"mId": image_id}
        if self.atlas_id:
            meta_value["atlasId"] = self.atlas_id
        write_meta(abs_path + ".meta", "o2::ImageAsset::Meta", meta_value)

        self.images.append(rel_path)
        return image_id, rel_path

    def center(self, bbox):
        """PSD bbox center -> o2 world position (canvas center origin, y up)."""
        cx = (bbox[0] + bbox[2]) * 0.5
        cy = (bbox[1] + bbox[3]) * 0.5
        return ((cx - self.canvas[0] * 0.5) * self.scale,
                (self.canvas[1] * 0.5 - cy) * self.scale)

    def transform(self, bbox, parent_center):
        center = self.center(bbox)
        return {
            "position": vec(center[0] - parent_center[0], center[1] - parent_center[1]),
            "size": vec((bbox[2] - bbox[0]) * self.scale, (bbox[3] - bbox[1]) * self.scale),
            "scale": vec(1.0, 1.0),
            "pivot": vec(0.5, 0.5),
        }, center

    def actor_value(self, layer, parent_center):
        """Actor payload for a layer or group; None when there is nothing to build."""
        if not self.include_hidden and not layer.visible:
            return None
        if layer.width == 0 or layer.height == 0:
            return None

        bbox = layer.bbox
        transform, center = self.transform(bbox, parent_center)
        value = {
            "Id": new_id(),
            "mName": psd_lib.layer_name(layer),
            "Transform": transform,
        }
        if not layer.visible:
            value["mEnabled"] = False

        if layer.is_group():
            children = [self.actor_value(child, center) for child in layer]
            children = [child for child in children if child is not None]
            if not children:
                return None
            value["Children"] = [{"Type": "o2::Actor", "Data": child} for child in children]
        else:
            image_id, rel_path = self.save_layer_image(layer)
            if image_id is None:
                return None

            component = {"mImageAsset": {"id": image_id, "path": rel_path}, "mId": new_id()}
            if layer.opacity != 255:
                component["mColor"] = {"r": 255, "g": 255, "b": 255, "a": int(layer.opacity)}
            value["Components"] = [{"Type": "o2::ImageComponent", "Data": component}]

        return value

    def build(self, psd_path, prefab_name=None):
        psd = psd_lib.load_psd(psd_path)
        self.canvas = (psd.width, psd.height)

        name = prefab_name or os.path.splitext(os.path.basename(psd_path))[0]

        children = [self.actor_value(layer, (0.0, 0.0)) for layer in psd]
        children = [child for child in children if child is not None]

        root = {
            "Id": new_id(),
            "mName": name,
            "Transform": {
                "position": vec(0.0, 0.0),
                "size": vec(self.canvas[0] * self.scale, self.canvas[1] * self.scale),
                "scale": vec(1.0, 1.0),
                "pivot": vec(0.5, 0.5),
            },
            "Children": [{"Type": "o2::Actor", "Data": child} for child in children],
        }

        proto_rel = self.out_dir + "/" + name + ".proto"
        proto_abs = os.path.join(self.assets_root, proto_rel)
        os.makedirs(os.path.dirname(proto_abs), exist_ok=True)
        with open(proto_abs, "w") as f:
            json.dump({"mActor": {"Type": "o2::Actor", "Value": root}}, f, indent=4)
            f.write("\n")

        write_meta(proto_abs + ".meta", "o2::DefaultAssetMeta<o2::ActorAsset>", {"mId": new_uid()})

        return {"proto": proto_rel, "images": self.images, "canvas": list(self.canvas)}


def build_prefab(psd_path, out_dir, assets_root="Assets", atlas="Basic.atlas",
                 scale=1.0, include_hidden=False, prefab_name=None):
    builder = PrefabBuilder(assets_root, out_dir, atlas, scale, include_hidden)
    return builder.build(psd_path, prefab_name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("psd")
    parser.add_argument("out_dir", help="Folder under the assets root for images and the .proto")
    parser.add_argument("--assets-root", default="Assets")
    parser.add_argument("--atlas", default="Basic.atlas",
                        help="Atlas asset (relative to assets root) for extracted images; '' for none")
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--include-hidden", action="store_true")
    parser.add_argument("--name", default=None, help="Prefab name (default: PSD file name)")
    args = parser.parse_args()

    result = build_prefab(args.psd, args.out_dir, args.assets_root, args.atlas or None,
                          args.scale, args.include_hidden, args.name)
    print(json.dumps(result, indent=1))


if __name__ == "__main__":
    main()
