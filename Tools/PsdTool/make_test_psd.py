#!/usr/bin/env python3
"""Regenerates testdata/ui_mock.psd used by check_tools.py (pip install pytoshop).

Canvas 800x600: Background, group Header (HeaderBack + Title), group Panel
(PanelBack + nested group Buttons with PlayBtn/ExitBtn), hidden Debug layer.
"""

import os

import numpy as np
from pytoshop import enums
from pytoshop.user import nested_layers


def rect_layer(name, left, top, width, height, rgba, visible=True, opacity=255):
    r, g, b, a = rgba
    def chan(value):
        return np.full((height, width), value, dtype=np.uint8)
    return nested_layers.Image(
        name=name, visible=visible, opacity=opacity, top=top, left=left,
        channels={0: chan(r), 1: chan(g), 2: chan(b), -1: chan(a)})


def main():
    layers = [
        # pytoshop takes layers in Photoshop UI order: first entry is the topmost
        rect_layer("Debug", 10, 10, 100, 100, (255, 0, 255, 255), visible=False),
        nested_layers.Group(name="Panel", layers=[
            nested_layers.Group(name="Buttons", layers=[
                rect_layer("ExitBtn", 420, 380, 120, 60, (200, 60, 60, 255)),
                rect_layer("PlayBtn", 260, 380, 120, 60, (60, 200, 90, 255)),
            ]),
            rect_layer("PanelBack", 200, 200, 400, 300, (240, 220, 180, 255), opacity=230),
        ]),
        nested_layers.Group(name="Header", layers=[
            rect_layer("Title", 300, 30, 200, 60, (255, 255, 255, 255)),
            rect_layer("HeaderBack", 0, 0, 800, 120, (70, 90, 160, 255)),
        ]),
        rect_layer("Background", 0, 0, 800, 600, (40, 40, 48, 255)),
    ]

    psd = nested_layers.nested_layers_to_psd(layers, color_mode=enums.ColorMode.rgb,
                                             size=(800, 600))  # (height, width)? verified: canvas reads 800x600
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "testdata", "ui_mock.psd")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "wb") as f:
        psd.write(f)
    print("written", out)


if __name__ == "__main__":
    main()
