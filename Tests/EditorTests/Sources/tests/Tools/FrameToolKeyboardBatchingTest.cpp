#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// FrameTool's keyboard arrow handling now coalesces a held-arrow stream into
// ONE TransformAction (BeginKeyboardAction → AppendKeyboardStep* → EndKeyboardAction)
// instead of emitting one action per key press. This test simulates that pattern
// at the action layer — the FrameTool helpers are thin glue around exactly this
// shape — and asserts the regression-critical invariants:
//
//   1. N consecutive arrow steps land as ONE entry on the undo list (not N).
//   2. A single Undo of that entry jumps from the final pose all the way back
//      to the pre-drag pose.
//   3. The entry's Redo replays the full coalesced span.
//
// Before the batching fix, FrameTool::OnKeyPressed/OnKeyStayDown produced an
// action per call — so a 200-ms arrow hold filled the undo stack with dozens of
// 1-pixel actions and Ctrl+Z only walked back one pixel at a time.
TEST(FrameToolKeyboardBatching, ArrowStreamProducesOneCoalescedUndoEntry)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto list = mmake<ActionsList>();
    int undoBefore = list->GetUndoActionsCount();

    // BeginKeyboardAction: open one TransformAction for the whole arrow run.
    auto kbAction = mmake<TransformAction>(AsEditable({ a }));

    // AppendKeyboardStep × 5: each tick of the held arrow appends a 1-pixel step.
    for (int i = 0; i < 5; i++)
    {
        auto step = mmake<TransformAction>(AsEditable({ a }));
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].transform.origin += Vec2F(1.0f, 0.0f);
        kbAction->Append(step);
    }
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(5.0f, 0.0f)))
        << "Live mutation must apply step-by-step as Append(step) calls step->Redo().";

    // EndKeyboardAction: snapshot the final state and push to the undo list.
    kbAction->Completed();
    list->DoneAction(kbAction);

    EXPECT_EQ(list->GetUndoActionsCount(), undoBefore + 1)
        << "Held-arrow run must coalesce into ONE undo entry, not one per key press.";

    list->UndoAction();
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(0.0f, 0.0f)))
        << "Undo must jump from the final 5-pixel pose back to the pre-batch pose in one shot.";

    list->RedoAction();
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(5.0f, 0.0f)))
        << "Redo must replay the full coalesced span.";
}

// Body-frame drag interrupted by press-break: FrameTool::OnCursorPressBreak now
// calls HandleReleased so the partial drag lands on the undo stack instead of
// leaking. This simulates the fixed flow at the action layer — a partially
// completed drag, then the same Completed/DoneAction sequence the real
// HandleReleased runs.
//
// Before the fix, OnCursorPressBreak just cleared mIsDragging and discarded
// the in-flight mTransformAction — leaving visual changes in the world without
// a corresponding undo entry.
TEST(FrameToolKeyboardBatching, PressBreakDuringBodyDragStillCommitsToUndo)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto list = mmake<ActionsList>();
    int undoBefore = list->GetUndoActionsCount();

    // HandlePressed: create the in-flight action.
    auto dragAction = mmake<TransformAction>(AsEditable({ a }));

    // OnCursorStillDown × 3: each tick appends a step and mutates the world.
    for (int i = 0; i < 3; i++)
    {
        auto step = mmake<TransformAction>(AsEditable({ a }));
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].transform.origin += Vec2F(7.0f, 0.0f);
        dragAction->Append(step);
    }
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(21.0f, 0.0f)));

    // OnCursorPressBreak → HandleReleased: commit the in-flight drag.
    dragAction->Completed();
    list->DoneAction(dragAction);

    EXPECT_EQ(list->GetUndoActionsCount(), undoBefore + 1)
        << "Press-break must commit the partial drag to the undo stack.";

    list->UndoAction();
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(0.0f, 0.0f)))
        << "Undo of the press-broken drag must restore the pre-drag pose.";
}
