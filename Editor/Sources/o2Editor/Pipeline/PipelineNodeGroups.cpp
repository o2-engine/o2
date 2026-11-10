#include "o2Editor/stdafx.h"
#include "PipelineNodeGroups.h"

#include <algorithm>

namespace Editor
{
    namespace PipelineNodeGroups
    {
        const Vector<PipelineNodeGroup>& All()
        {
            static Vector<PipelineNodeGroup> groups = {
                { "io", "Input & output",
                  "Where a pipeline starts (a picture, a text, a sound you give it) and where it ends (the results to download)",
                  { "sourceImage", "sourceText", "sourceAudio", "demoPhrases", "finishImage", "finishText", "finishAudio", "finishVideo", "demoLogin" } },
                { "aiImage", "AI images",
                  "Pictures made or changed by a model: generate, edit, cut a part out, remove the background, upscale",
                  { "nanoBananaGen", "imageEdit", "imageExtract", "aiRemoveBg", "aiUpscale" } },
                { "imageTools", "Image tools",
                  "Local picture work, no model and no tokens: draw, compose layers, colour, gradient, outline, shadow",
                  { "drawImage", "imageCrop", "composer", "imageColor", "imageGradient", "imageOutline", "imageShadow" } },
                { "text", "Text",
                  "Texts and prompts: write or edit with a model, build a prompt, join texts",
                  { "aiText", "promptGen", "textEdit", "textConcat" } },
                { "gameUi", "Game UI",
                  "Game interface: a mockup to sprites and a layout, a UI kit, text styles, the AI UI layout",
                  { "imageToUi", "uiKit", "textStyle", "uiLayout" } },
                { "av", "Audio & video",
                  "Voice, sound effects, music, video, and audio processing",
                  { "ttsSpeech", "sfxGen", "musicGen", "videoGen", "audioProcess" } },
            };
            return groups;
        }

        String GroupOf(const PipelineNodeSchema& schema)
        {
            for (auto& group : All())
            {
                if (group.types.Contains(schema.type))
                    return group.id;
            }

            auto c = schema.category;
            if (c == PipelineNodeCategory::Source || c == PipelineNodeCategory::Output || c == PipelineNodeCategory::Flow)
                return "io";

            if (!schema.outputs.IsEmpty())
            {
                auto out = schema.outputs[0].portType;
                if (out == PipelinePortType::Text)
                    return "text";

                if (out == PipelinePortType::Audio || out == PipelinePortType::Video)
                    return "av";
            }

            return c == PipelineNodeCategory::AI ? "aiImage" : "imageTools";
        }

        bool IsResultNode(const PipelineNodeSchema& schema)
        {
            return schema.category == PipelineNodeCategory::Output || schema.category == PipelineNodeCategory::Flow;
        }

        Vector<Entry> Group(const Vector<const PipelineNodeSchema*>& schemas)
        {
            Vector<Entry> res;
            for (auto& group : All())
            {
                Entry entry;
                entry.group = &group;
                for (auto schema : schemas)
                {
                    if (schema && !schema->retired && GroupOf(*schema) == group.id)
                        entry.items.Add(schema);
                }

                auto rank = [&](const PipelineNodeSchema* s)
                {
                    int i = group.types.IndexOf(s->type);
                    return i < 0 ? group.types.Count() : i;
                };
                std::stable_sort(entry.items.begin(), entry.items.end(), [&](const PipelineNodeSchema* a, const PipelineNodeSchema* b)
                {
                    int ra = rank(a), rb = rank(b);
                    return ra != rb ? ra < rb : a->label.ToLowerCase() < b->label.ToLowerCase();
                });

                // The inputs before the results
                if (group.id == "io")
                {
                    std::stable_sort(entry.items.begin(), entry.items.end(), [](const PipelineNodeSchema* a, const PipelineNodeSchema* b)
                    {
                        return (int)IsResultNode(*a) < (int)IsResultNode(*b);
                    });
                }

                if (!entry.items.IsEmpty())
                    res.Add(entry);
            }

            return res;
        }
    }
}
