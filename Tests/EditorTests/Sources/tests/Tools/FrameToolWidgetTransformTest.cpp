#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Widget> MakeWidget(const Vec2F& position, const Vec2F& size)
    {
        auto w = mmake<Widget>(ActorCreateMode::InScene);
        w->layout->anchorMin = Vec2F(0.0f, 0.0f);
        w->layout->anchorMax = Vec2F(0.0f, 0.0f);
        w->layout->offsetMin = position;
        w->layout->offsetMax = position + size;
        TickScene();
        return w;
    }

    Vector<Ref<SceneEditableObject>> AsEditableW(std::initializer_list<Ref<Widget>> widgets)
    {
        Vector<Ref<SceneEditableObject>> v;
        for (auto& w : widgets)
            v.Add(DynamicCast<SceneEditableObject>(w));
        return v;
    }
}

// REPRO: dragging a Widget body via FrameTool / MoveTool / RotateTool / ScaleTool
// goes through TransformAction's AppendTransformStep → step->Redo() →
// TransformAction::SetTransforms. That helper applies SetTransform first,
// then SetLayout. For Widgets, SetTransform shifts offsets via
// WidgetLayout::UpdateOffsetsByCurrentTransform — but the SetLayout that
// follows resets anchorMin/Max AND offsetMin/Max to the captured BEFORE
// values, undoing the shift. Net effect: a Widget's basis change in a
// TransformAction is silently reverted on every Append step.
//
// Pre-fix this test FAILS — the widget refuses to translate.
TEST(FrameToolWidgetTransform, WidgetBodyTranslationViaActionActuallyMovesIt)
{
    SceneCleanGuard guard;
    auto w = MakeWidget(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    Vec2F posBefore = w->layout->GetWorldRect().LeftBottom();

    auto action = mmake<TransformAction>(AsEditableW({ w }));

    // Mirror what FrameTool::AppendTransformStep emits during a body-frame drag:
    // a step whose done.transform is before.transform translated by delta, with
    // done.layout left equal to before.layout.
    auto step = mmake<TransformAction>(AsEditableW({ w }));
    step->doneTransforms = step->beforeTransforms;
    step->doneTransforms[0].transform.origin += Vec2F(50.0f, 0.0f);
    action->Append(step);
    TickScene();

    Vec2F posAfter = w->layout->GetWorldRect().LeftBottom();
    EXPECT_TRUE(NearV(posAfter, posBefore + Vec2F(50.0f, 0.0f)))
        << "TransformAction Redo must actually translate the widget. Pre-fix the "
           "captured layout's offsets reset and the widget snaps back to its origin.";
}

// Same scenario, but verifies the round-trip: Undo restores pre-drag pose,
// Redo replays the move. Pre-fix Undo and Redo end up looking identical because
// Redo never moved the widget in the first place.
TEST(FrameToolWidgetTransform, WidgetBodyTranslationRoundTrip)
{
    SceneCleanGuard guard;
    auto w = MakeWidget(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    Vec2F posBefore = w->layout->GetWorldRect().LeftBottom();

    auto action = mmake<TransformAction>(AsEditableW({ w }));
    auto step = mmake<TransformAction>(AsEditableW({ w }));
    step->doneTransforms = step->beforeTransforms;
    step->doneTransforms[0].transform.origin += Vec2F(70.0f, 30.0f);
    action->Append(step);
    action->Completed();
    TickScene();

    action->Undo();
    TickScene();
    EXPECT_TRUE(NearV(w->layout->GetWorldRect().LeftBottom(), posBefore))
        << "Undo must put the widget back where it started.";

    action->Redo();
    TickScene();
    EXPECT_TRUE(NearV(w->layout->GetWorldRect().LeftBottom(), posBefore + Vec2F(70.0f, 30.0f)))
        << "Redo must replay the translation.";
}

// Coalesced sequence of body-drag steps (what FrameTool::OnCursorStillDown
// emits frame-by-frame): every step carries its own transform delta. Each
// Append → Redo must accumulate visually, not silently revert.
TEST(FrameToolWidgetTransform, WidgetBodyDragCoalescesAndAdvancesEachStep)
{
    SceneCleanGuard guard;
    auto w = MakeWidget(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    Vec2F posBefore = w->layout->GetWorldRect().LeftBottom();

    auto editable = AsEditableW({ w });
    auto main = mmake<TransformAction>(editable);

    // Each iteration captures current state as the new step's before, then bumps
    // origin by +10 — mirrors how FrameTool::AppendTransformStep emits a per-frame
    // delta during a body-drag.
    for (int i = 0; i < 4; i++)
    {
        auto step = mmake<TransformAction>(editable);
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].transform.origin += Vec2F(10.0f, 0.0f);
        main->Append(step);
        TickScene();

        Vec2F expected = posBefore + Vec2F(10.0f * (i + 1), 0.0f);
        EXPECT_TRUE(NearV(w->layout->GetWorldRect().LeftBottom(), expected))
            << "Step " << i << ": widget must reach " << expected.x << "," << expected.y
            << ", got " << w->layout->GetWorldRect().LeftBottom().x << ","
            << w->layout->GetWorldRect().LeftBottom().y;
    }
}
