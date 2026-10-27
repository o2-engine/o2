#pragma once

#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRun.h"

namespace Editor
{
    // Writes the prototype of an AI image to UI result into the assets (Assets/<assetPath>/<screenName>.proto with its
    // resources) from the layout and the sprites it names; logs what it wrote and what the widgets do not reproduce
    Coroutine<bool> ImageToUiExport(Ref<PipelineExecContext> ctx, PipelineImageToUiConfig cfg, String layout,
                                    Vector<UiLayout::ImageToUiSprite> sprites);

    // Registers the AI image to UI node
    void RegisterImageToUiNode();
}
