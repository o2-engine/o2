#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiLayout
{
    // Outlines a box
    void StrokeRect(UiImage& img, const Box& b, const Rgb& rgb, int width = 1);

    // Writes a short caption with its top-left at (x, y), outlined in black; ASCII drawn by a 5x7 pixel font
    void DrawCaption(UiImage& img, const String& text, double x, double y, const Rgb& rgb, int size = 11);

    // One picture of a strip with its caption
    struct StripEntry
    {
        String  title;
        UiImage img;
    };

    // Pictures side by side with captions, the same height
    UiImage Strip(const Vector<StripEntry>& entries, int gap = 8);

    // Outline colour of an element kind in the debug overlays
    Rgb KindColor(const String& kind);
}
