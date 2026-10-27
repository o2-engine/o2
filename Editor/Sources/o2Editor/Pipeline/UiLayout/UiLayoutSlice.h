#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutModel.h"

namespace Editor::UiLayout
{
    // Nine-slice margins of a sprite, source pixels
    struct NineSlice
    {
        int left = 0, top = 0, right = 0, bottom = 0;
    };

    // What is drawn over an element and must not end up in its sprite
    struct EraseItem
    {
        String label; // How the cut prompt names it
        Box    box;
        String color; // Text colour, empty for the rest
    };

    // One look to cut: the instance it is cut from and every element drawn with it
    struct LookPlan
    {
        String            key;
        String            describe;
        Ref<UiEl>         rep;
        Vector<Ref<UiEl>> instances;
        Vector<EraseItem> erase; // What the model must remove from the representative
        bool              stretch = false;
    };

    // A debug picture of a cut
    struct SpriteStep
    {
        String  title;
        UiImage img;
    };

    // ------------------------------------------------------------------------------------------------
    // A sprite: the design's own pixels with the model's matte, where it sits on its representative, how
    // it was made and the pictures of the steps
    // ------------------------------------------------------------------------------------------------
    struct SpriteCut: public RefCounterable
    {
        String                   key;
        UiImage                  img;
        Box                      bounds; // Where the trimmed sprite sits on the representative, design pixels
        String                   method; // local, ai
        std::optional<NineSlice> nine;
        Vector<SpriteStep>       steps;
        String                   note;
    };

    // Groups every look-drawing element into looks of one size each and picks the instance each look is cut from
    Vector<LookPlan> PlanLooks(UiDoc& doc);

    // What is drawn over an element: its descendants, later elements lying mostly on it, labels crossing it
    Vector<EraseItem> EraseList(const UiDoc& doc, const Ref<UiEl>& e);

    // A chroma key colour
    struct KeyColor
    {
        String name;
        String hex;
        Rgb    rgb;
    };

    // The key colour farthest (in chrominance) from the element's own colours
    KeyColor PickKey(const UiImage& img, const Box& b);

    // Nine-slice margins: a bit more than the corner rounding read along the diagonals, at most 40 % of a side
    NineSlice NineOf(const UiImage& img);

    // Result of a cut
    struct CutResult
    {
        bool           ok = false;
        String         error;
        Ref<SpriteCut> cut;
    };

    // Cuts one look: a flat backdrop is keyed out locally (sprites auto), else the model draws its matte
    Coroutine<CutResult> CutLook(const Ref<AiCtx>& ai, const UiImage* design, const UiDoc* doc, const LookPlan* plan, String mode, int seed);

    // Per-axis scale and shift laying a model render over the crop
    struct Registration
    {
        double sx = 1, sy = 1, tx = 0, ty = 0;
    };

    // The scale and shift that lay the model's render over the crop: identity unless its opaque box differs clearly from the
    // analysis box, then refined where the signed gradients correlate best
    Registration Register(const UiImage& render, const UiImage& crop, const Box& rough, const Vector<Box>& erase);

    // The render resampled through the registration into a w x h frame
    UiImage Warp(const UiImage& src, const Registration& r, int w, int h);

    // Pulls the key's own channels down to the others' level where the matte is partial or within 2 px of transparency
    void Despill(UiImage& img, const Rgb& key);

    // Alpha from the render; colour from the crop where the element is opaque and untouched, from the colour-matched render
    // under erased overlays and on soft edges, everywhere when renderColors
    UiImage Composite(const UiImage& crop, const UiImage& render, const Vector<Box>& erase, bool renderColors);

    // Fills the boxes the model left drawn from their surroundings: a label whose colour is still there, an overlay that still
    // looks like the design; returns what was filled
    Vector<String> CleanLeftovers(UiImage& img, const UiImage& crop, const Vector<EraseItem>& labels, const Vector<Box>& boxes);

    // Fills the inside of [x0..x1] x [y0..y1] from its border by 400 Jacobi sweeps, opaque pixels only; fromBorder starts
    // the inside at the border's mean (a whole glyph to remove) instead of the pixels themselves
    void HarmonicFill(UiImage& img, int x0, int y0, int x1, int y1, bool fromBorder = false);

    // Number of UTF-16 code units of a UTF-8 text, what JavaScript's length counts
    int Utf16Length(const String& text);
}
