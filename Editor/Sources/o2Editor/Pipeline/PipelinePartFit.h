#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // How large a cut-out part really is against the picture it was cut from (port of AssetsLine
    // backend/src/nodes/partFit.ts). An image model is given a crop and asked to keep the element in place, but models
    // redraw: an element often comes back clearly smaller or larger inside its frame than the crop shows it, and then
    // the frame's ratio misstates the part's density. FitPartOnCrop lays the trimmed sprite over the crop at every size
    // from 0.45 to 1.6 of the nominal one and every place, scored by how its own pixels fit the crop there and by whether
    // the crop changes across its outline; the nominal size stays unless another one is clearly better (a smaller one has
    // to do more: fit closely in colour and end on a clear boundary)
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelinePartFit
    {
        // ----------------------------------------------------------------
        // The measured scale and where the sprite lies on the crop at it
        // ----------------------------------------------------------------
        struct Fit
        {
            double scale = 1;                // Sprite pixels per crop pixel
            double x = 0, y = 0, w = 0, h = 0; // Where the sprite lies on the crop at that scale, crop pixels
            double ratio = 1;                // The measured scale against the nominal one
        };

        // Measures the sprite against its crop. nominal: the scale the generation claims. None: nothing to measure by (an
        // opaque plate, a speck) or no size is clearly better than the nominal one. Answers are memoised by content
        std::optional<Fit> FitPartOnCrop(const UiLayout::UiImage& crop, const UiLayout::UiImage& sprite, double nominal);
    }
}
