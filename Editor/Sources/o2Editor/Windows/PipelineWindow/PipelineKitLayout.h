#pragma once

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

// ------------------------------------------------------------------------------------------------
// The UI kit card's geometry (port of AssetsLine kitLayout.ts): the device row, the head (summary
// + AI, the models) and a grid of cells - one per input (picture or text style), then the "+ input"
// cell. Like the AI extract's parts grid mirrored: each cell carries its INPUT port on the top-left
// corner of its picture. The grid never scrolls and takes as many columns as the card's width
// holds, so the card's height and every port's place come from these numbers. Body coordinates:
// x from the card's left edge, y down from the body's top
// ------------------------------------------------------------------------------------------------
namespace Editor::PipelineKitLayout
{
    constexpr float bodyTop = 6.0f;    // The body's first row
    constexpr float padX = 11.0f;      // The body's side padding
    constexpr float deviceH = 26.0f;   // The target device's row (label + chip), above the head
    constexpr float headH = 28.0f;     // Summary + AI
    constexpr float modelsH = 26.0f;   // A models row
    constexpr float gap = 6.0f;        // Between the rows
    constexpr float modelsWide = 400.0f; // A card narrower than this inside puts "remove backgrounds" on a second models row
    constexpr float cellGap = 6.0f;
    constexpr float cellMin = 76.0f;   // Square pictures of at least this (more columns first)...
    constexpr float cellMax = 116.0f;  // ...and at most this
    constexpr float capGap = 2.0f;     // Under a picture: 2 px, the name, the type line
    constexpr float nameH = 18.0f;
    constexpr float metaH = 13.0f;
    constexpr float defaultWidth = 460.0f; // The card's default width

    // --------------------------------------------------------------------------------------
    // The grid of a card: the inputs and the slots they take with the add cell, the columns and
    // rows, a cell's picture side and its whole height, the models rows, the grid's top and height
    // --------------------------------------------------------------------------------------
    struct Grid
    {
        int   n = 0, slots = 1;
        int   cols = 1, rows = 1;
        float cell = 76.0f, cellH = 109.0f;
        int   modelRows = 1;
        float top = 0.0f, h = 0.0f;
    };

    // ------------------------------------------
    // A box in body coordinates: x, y down, w, h
    // ------------------------------------------
    struct Box
    {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    };

    // The grid of a card of the width with that many inputs
    inline Grid Of(float cardWidth, int inputs)
    {
        Grid g;
        float inner = cardWidth - 2.0f*padX;
        g.n = inputs;
        g.slots = inputs + 1;
        g.cols = Math::Max(1, (int)Math::Floor((inner + cellGap)/(cellMin + cellGap)));
        g.cell = Math::Max(24.0f, Math::Min(cellMax, Math::Floor((inner - cellGap*(g.cols - 1))/g.cols)));
        g.rows = (g.slots + g.cols - 1)/g.cols;
        g.cellH = g.cell + capGap + nameH + metaH;
        g.modelRows = inner >= modelsWide ? 1 : 2;
        g.top = bodyTop + deviceH + gap + headH + g.modelRows*(gap + modelsH) + gap;
        g.h = g.rows*g.cellH + (g.rows - 1)*cellGap;
        return g;
    }

    // The body's height: the grid's top and the grid
    inline float BodyHeight(float cardWidth, int inputs)
    {
        Grid g = Of(cardWidth, inputs);
        return g.top + g.h;
    }

    // Slot i's picture box, body coordinates
    inline Box CellBox(const Grid& g, int i)
    {
        int col = i % g.cols, row = i/g.cols;
        return Box{ padX + col*(g.cell + cellGap), g.top + row*(g.cellH + cellGap), g.cell, g.cell };
    }

    // Slot i's picture box inside the grid (x and y from the grid's top-left corner)
    inline Box CellInGrid(const Grid& g, int i)
    {
        int col = i % g.cols, row = i/g.cols;
        return Box{ col*(g.cell + cellGap), row*(g.cellH + cellGap), g.cell, g.cell };
    }

    // Where the input at index sits - the top-left corner of its cell's picture, body coordinates; a port no cell shows
    // (index < 0) sits on the add cell
    inline Vec2F InputPos(const Grid& g, int index)
    {
        Box b = CellBox(g, index < 0 ? g.n : index);
        return Vec2F(b.x, b.y);
    }
}
