#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/CurveEditor/CurvesEditor.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct CurvesEditorProbe : CurvesEditor
    {
        using CurvesEditor::CurvesEditor;
        using CurvesEditor::mTransformFrameVisible;
    };
}

// A selection made by code, not by the cursor, shows the frame around the keys too
TEST(CurvesEditorSelectionUI, SelectAllShowsTheFrame)
{
    PushEditorScopeOnStack scope;

    auto editor = mmake<CurvesEditorProbe>();
    editor->AddCurve("curve", mmake<Curve>(Vector<Vec2F>({ Vec2F(0.0f, 0.0f), Vec2F(0.5f, 1.0f), Vec2F(1.0f, 0.0f) })));
    EXPECT_FALSE(editor->mTransformFrameVisible);

    editor->SelectAll();
    EXPECT_TRUE(editor->mTransformFrameVisible);

    editor->DeselectAll();
    EXPECT_FALSE(editor->mTransformFrameVisible);
}
