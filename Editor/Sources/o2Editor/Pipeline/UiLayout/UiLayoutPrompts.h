#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

// The prompts of the image-to-UI node's own part (the sliced sprites the skill takes as its input), word for word the
// strings of AssetsLine's backend/src/uiLayout/prompts.ts and the Unity plugin; change them together. The layout's prompts
// are the agent's (o2/Tools/ImageToUi/agent/agent.py)
namespace Editor::UiLayout::Prompts
{
    // The analysis of the mockup: background, looks and the element list
    String Analyze(int w, int h, const String& hint);

    // Cut one look out of its crop: the element stays, what is drawn on it goes, the rest becomes the key colour
    String Cut(const String& describe, const Vector<String>& erase, const String& keyName, const String& keyHex);

    // The background plate behind the UI
    String Background(const String& describe, bool strict = false);
}
