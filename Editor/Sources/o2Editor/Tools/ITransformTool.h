#pragma once

#include "o2Editor/Tools/SelectionTool.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class SceneEditableObject;
}

namespace Editor
{
    // ---------------------
    // Editor transform tool
    // ---------------------
    class ITransformTool: public SelectionTool
    {
    public:
        Function<void()> onTransformBegin; // Called when transform begins
        Function<void()> onTransformEnd;   // Called when transform ends

        IOBJECT(ITransformTool);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ITransformTool)
{
    BASE_CLASS(Editor::SelectionTool);
}
END_META;
CLASS_FIELDS_META(Editor::ITransformTool)
{
    FIELD().PUBLIC().NAME(onTransformBegin);
    FIELD().PUBLIC().NAME(onTransformEnd);
}
END_META;
CLASS_METHODS_META(Editor::ITransformTool)
{
}
END_META;
// --- END META ---
