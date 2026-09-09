#pragma once

#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"

namespace Editor
{
    // Creates the body of a finish, source, text tool, AI text, prompt, video or audio node; null for other types
    Ref<PipelineNodeBody> CreateBasicNodeBody(const String& type);

    // Creates the body of an image generation, edit, extract, background removal, effect or draw node; null for other types
    Ref<PipelineNodeBody> CreateImageNodeBody(const String& type);

    // Creates the composer body; null for other types
    Ref<PipelineNodeBody> CreateComposerNodeBody(const String& type);
}
