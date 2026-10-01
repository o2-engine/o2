#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class DataValue;
}

namespace Editor
{
    class PipelineNode;

    // A frame an image model renders
    struct PipelineRenderAspect
    {
        String label;        // "3:2"
        float  ratio = 1.0f; // Width over height

        bool operator==(const PipelineRenderAspect& other) const { return label == other.label; }
    };

    // -----------------------------------------------------------------------------------------------------------
    // AI upscale: the size the node produces and how the image model is asked to render it. The same rules as
    // AssetsLine's shared/upscale.ts, so the editors show the numbers the run uses
    // -----------------------------------------------------------------------------------------------------------
    namespace PipelineUpscale
    {
        constexpr int maxSide = 8192; // No side of the result is larger

        // Returns the size mode of the config: x2, x3, x4 or size; x2 for anything else
        String ModeOf(const PipelineNode& node);

        // Returns the size the node makes from an input of the size: x2/x3/x4 scale both sides; "size" takes targetW and,
        // unless the aspect is unlocked, the height from the input's shape. Capped at maxSide keeping the shape, rounded
        Vec2I Target(const PipelineNode& node, const Vec2I& input);

        // Returns the frames a model renders
        const Vector<PipelineRenderAspect>& RenderAspects();

        // Returns the frame closest to the size, by the logarithm of the ratio
        PipelineRenderAspect ClosestAspect(float width, float height);

        // Returns the smallest output resolution whose long side covers the length: 1K, 2K or 4K
        String RenderSizeFor(float longSide);

        // Returns true for the image models that render 2K / 4K themselves: the Gemini 3 image models, directly or
        // through OpenRouter
        bool RendersLargeSizes(const String& model);

        // Returns the prompt of the upscale call
        String Prompt(const String& details, bool transparentInput);

        // Note of the card for a model that answers at its own size
        extern const String smallModelNote;
    }
}
