#pragma once

#include "o2Editor/Pipeline/PipelineImageOps.h"

using namespace o2;

namespace o2
{
    class DataValue;
}

namespace Editor
{
    class PipelineNode;

    // --------------------------------------------------------------------------------------------------
    // Edit region of an AI image edit node ("editRegion", fractions of the input): only that frame changes.
    // The model gets a crop around it and its answer is pasted back with a feathered margin. The same rules
    // as AssetsLine's editRegionOf / regionBoxes / pasteRegionEdit
    // --------------------------------------------------------------------------------------------------
    namespace PipelineEditRegion
    {
        // Rectangle in image pixels, right and bottom exclusive
        struct PixelBox
        {
            int left = 0, top = 0, right = 0, bottom = 0;

            int Width() const { return right - left; }
            int Height() const { return bottom - top; }
        };

        // The region in pixels and the context box sent to the model around it
        struct Boxes
        {
            PixelBox region; // Frame the change is made in
            PixelBox box;    // Region grown by the margin, clamped to the image
        };

        constexpr float minSize = 0.02f; // A region narrower or lower than this is none

        extern const String promptSuffix; // Ending of the edit prompt when the model gets the crop around the region

        // Reads a region value: four finite numbers, x and y clamped to 0..1, w and h to what is left; false for none
        bool Read(const DataValue* value, PipelineImageOps::CropRect& region);

        // Reads the region of an image edit node; false for any other node or when it has none
        bool Of(const PipelineNode& node, PipelineImageOps::CropRect& region);

        // Returns the region in pixels of an image of the size and the context box around it: the region grown by
        // max(16, a quarter of its longer side) on each side, clamped to the image
        Boxes PixelBoxes(const Vec2I& imageSize, const PipelineImageOps::CropRect& region);

        // Returns the blend weight of a pixel of the context box: 1 inside the region, falling linearly to 0 at the box
        // border across the margin on each side; a side without margin has no pixels outside the region
        float Weight(int x, int y, const Boxes& boxes);

        // Pastes the model's edit of the context box back into the input: the edit is stretched to the box and every
        // channel, alpha included, is blended by Weight; pixels outside the box stay the input's
        Ref<Bitmap> PasteRegionEdit(const Bitmap& input, const Bitmap& edited, const Boxes& boxes);
    }
}
