#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"

// ------------------------------------------------------------------------------------------------
// What UiDocAi.cpp and UiDocAiKit.cpp share; not for other users
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // A text model call: the test stand-in when one is set, else AiRouter::GenerateText
    Coroutine<AiTextResult> UiAiText(Ref<PipelineExecContext> ctx, String model, String prompt, Vector<AiImageRef> images);
}
