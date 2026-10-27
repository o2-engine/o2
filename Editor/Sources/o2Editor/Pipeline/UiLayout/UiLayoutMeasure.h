#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutModel.h"

namespace Editor::UiLayout
{
    // The em size at which a Roboto text shows a visible glyph height of inkH. The texts are measured only to know what a
    // sprite cut must erase, so the cap height stands in for the glyphs' own
    double FontSizeForInk(double inkH);

    // The dominant colour of a region: the most populated 4-bit-per-channel bin, averaged
    Rgb DominantColor(const UiImage& img, const Box& b, int skipAlphaBelow = 128);

    // Measures a free-standing text by its ink inside a slightly grown analysis box
    void MeasureText(const UiImage& img, UiEl& el);

    // Measures the texts of one grid cell: the biggest glyphs are the main label (slot 0), the smaller ones the
    // secondary labels
    void MeasureCellTexts(const UiImage& img, const UiEl& cell, const Vector<Ref<UiEl>>& texts);

    // The text box around measured ink: glyphs touch the aligned edge, a line's height around them
    Box TextBoxFor(const Box& ink, double fs, const String& align);
}
