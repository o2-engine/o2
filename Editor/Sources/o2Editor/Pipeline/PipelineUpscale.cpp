#include "o2Editor/stdafx.h"
#include "PipelineUpscale.h"

#include <cmath>
#include <regex>

#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::PipelineUpscale
{
    const String smallModelNote = "This model answers at about 1K: the rest is resampled. Gemini 3 image models render 2K / 4K themselves.";

    String ModeOf(const PipelineNode& node)
    {
        String mode = node.GetConfigString("upscale", "x2");
        return mode == "x3" || mode == "x4" || mode == "size" ? mode : String("x2");
    }

    static bool PositiveNumber(const PipelineNode& node, const char* key, double& value)
    {
        auto v = node.GetConfigValue(key);
        if (!v || !v->IsNumber())
            return false;

        value = (double)*v;
        return std::isfinite(value) && value > 0.0;
    }

    Vec2I Target(const PipelineNode& node, const Vec2I& input)
    {
        double iw = Math::Max(1, input.x), ih = Math::Max(1, input.y);
        String mode = ModeOf(node);
        double w, h;
        if (mode == "size")
        {
            if (!PositiveNumber(node, "targetW", w))
                w = iw*2.0;

            auto lock = node.GetConfigValue("lockAspect");
            bool unlocked = lock && lock->IsBoolean() && !(bool)*lock;
            if (unlocked)
            {
                if (!PositiveNumber(node, "targetH", h))
                    h = ih*2.0;
            }
            else
                h = w*ih/iw;
        }
        else
        {
            double f = mode == "x4" ? 4.0 : mode == "x3" ? 3.0 : 2.0;
            w = iw*f;
            h = ih*f;
        }

        double k = Math::Min(1.0, (double)maxSide/Math::Max(w, h));
        return Vec2I(Math::Max(1, (int)std::lround(w*k)), Math::Max(1, (int)std::lround(h*k)));
    }

    const Vector<PipelineRenderAspect>& RenderAspects()
    {
        static Vector<PipelineRenderAspect> aspects = {
            { "1:1", 1.0f }, { "2:3", 2.0f/3.0f }, { "3:2", 3.0f/2.0f }, { "3:4", 3.0f/4.0f }, { "4:3", 4.0f/3.0f },
            { "4:5", 4.0f/5.0f }, { "5:4", 5.0f/4.0f }, { "9:16", 9.0f/16.0f }, { "16:9", 16.0f/9.0f }, { "21:9", 21.0f/9.0f }
        };
        return aspects;
    }

    PipelineRenderAspect ClosestAspect(float width, float height)
    {
        double r = std::log(Math::Max(1e-6, (double)width)/Math::Max(1e-6, (double)height));
        auto& aspects = RenderAspects();
        PipelineRenderAspect best = aspects[0];
        for (auto& a : aspects)
        {
            if (std::abs(std::log((double)a.ratio) - r) < std::abs(std::log((double)best.ratio) - r))
                best = a;
        }
        return best;
    }

    String RenderSizeFor(float longSide)
    {
        return longSide <= 1100.0f ? String("1K") : longSide <= 2200.0f ? String("2K") : String("4K");
    }

    bool RendersLargeSizes(const String& model)
    {
        String id = model.Trimed(" \n\r\t");
        if (id.StartsWith("models/"))
            id = id.SubStr(7);

        static const std::regex pattern("(^|/)gemini-3(\\.\\d+)?-(pro|flash)-image");
        return std::regex_search(std::string(id.ToLowerCase().Data()), pattern);
    }

    String Prompt(const String& details, bool transparentInput)
    {
        Vector<String> parts = {
            "Upscale this image: redraw it at a higher resolution with more fine detail.",
            "It must stay the SAME picture - identical composition and framing, every shape, outline, position and proportion,",
            "the same colors, lighting and art style. Do not add, remove, move or restyle anything.",
            "Sharpen the edges and refine textures, materials and small details so it holds up at the larger size; no blur, no noise, no artifacts.",
            "Strips along the edges may be repeated edge pixels - keep them as they are."
        };
        if (transparentInput)
            parts.Add("The flat gray backdrop is not part of the subject - keep it flat gray.");
        if (!details.IsEmpty())
            parts.Add("Details to add and how: " + details);
        parts.Add("Output only the resulting image.");

        String result;
        for (auto& part : parts)
            result += (result.IsEmpty() ? String() : String(" ")) + part;
        return result;
    }
}
