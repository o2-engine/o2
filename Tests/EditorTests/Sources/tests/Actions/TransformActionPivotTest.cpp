#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Actor> MakeSizedActor(const Vec2F& pos, const Vec2F& size)
    {
        auto a = mmake<Actor>(ActorCreateMode::InScene);
        a->transform->SetPosition(pos);
        a->transform->SetSize(size);
        TickScene();
        return a;
    }

    Basis ActorBasis(const Ref<Actor>& a)
    {
        return static_cast<const Actor*>(a.Get())->GetTransform();
    }

    // Solve worldPivot = b.origin + b.xv*a + b.yv*b for relative coords —
    // mirrors the formula TransformAction uses internally.
    Vec2F WorldPivotToRelative(const Basis& b, const Vec2F& worldPivot)
    {
        float det = b.xv.x*b.yv.y - b.yv.x*b.xv.y;
        Vec2F p = worldPivot - b.origin;
        return Vec2F((p.x*b.yv.y - p.y*b.yv.x) / det,
                     (b.xv.x*p.y - b.xv.y*p.x) / det);
    }
}

// Capture-then-restore round-trip for a pivot-only edit (FrameTool drag of the
// pivot handle). Both Redo and Undo must restore the world pivot to the value
// it had when each side was captured.
TEST(TransformActionPivot, RedoUndoRoundTripRestoresPivot)
{
    SceneCleanGuard guard;
    auto a = MakeSizedActor(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    a->SetPivot(Vec2F(10.0f, 10.0f));
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({ a }));

    a->SetPivot(Vec2F(70.0f, 80.0f));
    TickScene();
    action->Completed();

    a->SetPivot(Vec2F(999.0f, 999.0f));
    TickScene();

    action->Redo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(70.0f, 80.0f)));

    action->Undo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(10.0f, 10.0f)));
}

// Pivot-only step must not move the actor's world basis. Mirrors what
// FrameTool::OnPivotHandle emits — only the relative pivot offset changes.
TEST(TransformActionPivot, PivotOnlyStepLeavesTransformUntouched)
{
    SceneCleanGuard guard;
    auto a = MakeSizedActor(Vec2F(50.0f, 60.0f), Vec2F(100.0f, 100.0f));

    Basis basisBefore = ActorBasis(a);

    auto step = mmake<TransformAction>(AsEditable({ a }));
    step->doneTransforms = step->beforeTransforms;
    step->doneTransforms[0].pivot = WorldPivotToRelative(basisBefore, Vec2F(120.0f, 130.0f));
    step->Redo();

    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(120.0f, 130.0f)));
    EXPECT_TRUE(ActorBasis(a) == basisBefore)
        << "Pivot-only action must not move the actor's world basis.";
}

// FrameTool emits incremental pivot steps as the user drags. They must coalesce
// into a single undoable action whose Undo jumps from the final pivot back to
// the pre-drag pivot in one shot.
TEST(TransformActionPivot, AppendCoalescesPivotSteps)
{
    SceneCleanGuard guard;
    auto a = MakeSizedActor(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    a->SetPivot(Vec2F(0.0f, 0.0f));
    TickScene();

    auto editable = AsEditable({ a });
    auto main = mmake<TransformAction>(editable);

    auto pushStep = [&](const Vec2F& worldPivot)
    {
        auto step = mmake<TransformAction>(editable);
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].pivot = WorldPivotToRelative(step->doneTransforms[0].transform, worldPivot);
        main->Append(step);
    };

    pushStep(Vec2F(10.0f, 0.0f));
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(10.0f, 0.0f)));

    pushStep(Vec2F(25.0f, 5.0f));
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(25.0f, 5.0f)));

    pushStep(Vec2F(40.0f, 12.0f));
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(40.0f, 12.0f)));

    ASSERT_EQ(main->doneTransforms.Count(), 1);

    a->SetPivot(Vec2F(-1.0f, -1.0f));
    TickScene();

    main->Undo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(0.0f, 0.0f)))
        << "Undo of coalesced pivot drag must jump straight to the pre-drag pivot.";

    main->Redo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(40.0f, 12.0f)))
        << "Redo must replay the final pivot of the coalesced span.";
}

// Pivot edits and translation edits live in the same TransformAction — the
// round-trip must restore both world position and world pivot.
TEST(TransformActionPivot, RoundTripWithTransformAndPivotChanged)
{
    SceneCleanGuard guard;
    auto a = MakeSizedActor(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    a->SetPivot(Vec2F(0.0f, 0.0f));
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({ a }));

    SetActorPos(a, Vec2F(70.0f, 0.0f));
    a->SetPivot(Vec2F(85.0f, 25.0f));
    TickScene();
    action->Completed();

    SetActorPos(a, Vec2F(-1.0f, -1.0f));
    a->SetPivot(Vec2F(-50.0f, -50.0f));
    TickScene();

    action->Undo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(0.0f, 0.0f)));

    action->Redo();
    EXPECT_TRUE(NearV(a->GetPivot(), Vec2F(85.0f, 25.0f)));
}

// Translation-only edit must leave the relative pivot unchanged: the world
// pivot follows the basis. Guards against the regression where TransformAction
// over-wrote the pivot back to its captured value, locking it to the old
// origin even when the basis had translated.
TEST(TransformActionPivot, TranslationOnlyMovesWorldPivotWithBasis)
{
    SceneCleanGuard guard;
    auto a = MakeSizedActor(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));
    Vec2F pivotBefore = a->GetPivot();

    auto action = mmake<TransformAction>(AsEditable({ a }));
    action->doneTransforms = action->beforeTransforms;
    action->doneTransforms[0].transform.origin += Vec2F(50.0f, 0.0f);
    action->Redo();

    EXPECT_TRUE(NearV(a->GetPivot(), pivotBefore + Vec2F(50.0f, 0.0f)))
        << "World pivot must follow a basis-only translation, not stay pinned.";
}
