#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutModel.h"

namespace Editor::UiLayout
{
    // A model's grid is a count and a box; its cells are the box divided evenly. A model that counts one column too many
    // (an empty one), or gives a box a row short, puts every cell off its tile, and the cuts slide with them. The picture
    // itself says where the cells are: the pitch is the period of its edges, the cell borders run through its gutters (the
    // lines of the steadiest colour). FitGrids moves a grid onto them when that makes its lines quieter than the model's.
    // AssetsLine's gridFit.ts

    // One axis of a grid as the picture shows it
    struct GridAxisFit
    {
        double pitch = 0; // Cell size, px
        double start = 0; // Where the first kept cell starts, px
        int    count = 0; // Cells kept
        int    shift = 0; // Model cells dropped before the first kept one
    };

    // Fits one axis of a grid: its span s0..s1 across b0..b1 of the other axis, count model cells of which occupied hold
    // something. Empty when the picture has no clear period there, or its lines are no better than the model's
    std::optional<GridAxisFit> FitGridAxis(const UiImage& img, bool alongX, double s0, double s1, double b0, double b1, int count,
                                           const std::set<int>& occupied);

    // Moves every grid of the analysis onto the picture's cells; the cells and their texts follow. Returns a line per grid
    // that moved
    Vector<String> FitGrids(UiDoc& doc, const UiImage& design);
}
