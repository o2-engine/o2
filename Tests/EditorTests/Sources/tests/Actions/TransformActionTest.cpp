#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/IAction.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Basis GetBasis(const Ref<Actor>& a)
    {
        return static_cast<const Actor*>(a.Get())->GetTransform();
    }
}

TEST(TransformAction, CtorCapturesBeforeTransforms)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 20.0f));

    Basis expected = GetBasis(a);
    auto action = mmake<TransformAction>(AsEditable({a}));

    ASSERT_EQ(action->beforeTransforms.Count(), 1);
    EXPECT_TRUE(action->beforeTransforms[0].transform == expected);
    EXPECT_EQ(action->doneTransforms.Count(), 0);
}

TEST(TransformAction, CompletedCapturesDoneTransforms)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({a}));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    Basis expected = GetBasis(a);
    action->Completed();

    ASSERT_EQ(action->doneTransforms.Count(), 1);
    EXPECT_TRUE(action->doneTransforms[0].transform == expected);
}

TEST(TransformAction, RedoUndoRestoreAndReplay)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({a}));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();

    SetActorPos(a, Vec2F(999.0f, 999.0f));

    action->Redo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(50.0f, 0.0f)));

    action->Undo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(10.0f, 0.0f)));
}

TEST(TransformAction, AppendMergesDoneTransformsAndAppliesStep)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({a}));

    SetActorPos(a, Vec2F(10.0f, 0.0f));
    auto step1 = mmake<TransformAction>(AsEditable({a}));
    step1->Completed();
    main->Append(step1);
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(10.0f, 0.0f)));

    SetActorPos(a, Vec2F(25.0f, 0.0f));
    auto step2 = mmake<TransformAction>(AsEditable({a}));
    step2->Completed();
    main->Append(step2);
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(25.0f, 0.0f)));

    ASSERT_EQ(main->doneTransforms.Count(), 1);

    SetActorPos(a, Vec2F(999.0f, 999.0f));
    main->Undo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(0.0f, 0.0f)));

    main->Redo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(25.0f, 0.0f)));
}

TEST(TransformAction, AppendAcrossMultipleObjects)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto b = MakeActor(Vec2F(100.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({a, b}));

    SetActorPos(a, Vec2F(5.0f, 0.0f));
    SetActorPos(b, Vec2F(105.0f, 0.0f));
    auto step = mmake<TransformAction>(AsEditable({a, b}));
    step->Completed();
    main->Append(step);

    SetActorPos(a, Vec2F(-1.0f, -1.0f));
    SetActorPos(b, Vec2F(-1.0f, -1.0f));

    main->Undo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(0.0f, 0.0f)));
    EXPECT_TRUE(NearV(b->transform->GetPosition(), Vec2F(100.0f, 0.0f)));

    main->Redo();
    EXPECT_TRUE(NearV(a->transform->GetPosition(), Vec2F(5.0f, 0.0f)));
    EXPECT_TRUE(NearV(b->transform->GetPosition(), Vec2F(105.0f, 0.0f)));
}

namespace
{
    class StubAction : public IAction
    {
    public:
        String GetName() const override { return "Stub"; }
    };
}

TEST(TransformAction, TryMergeRejectsNonTransformAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto main = mmake<TransformAction>(AsEditable({a}));

    auto stub = mmake<StubAction>();
    EXPECT_FALSE(main->TryMerge(stub));
}

TEST(TransformAction, TryMergeRejectsMismatchedObjectIds)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto b = MakeActor(Vec2F(0.0f, 0.0f));
    auto c = MakeActor(Vec2F(0.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({a, b}));
    auto step = mmake<TransformAction>(AsEditable({a, c}));
    step->Completed();

    EXPECT_FALSE(main->TryMerge(step));
}

TEST(TransformAction, RedoUpdatesWorldPivotImmediately)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetSize(Vec2F(100.0f, 100.0f));
    TickScene();

    Vec2F pivotBefore = a->GetPivot();

    auto action = mmake<TransformAction>(AsEditable({a}));
    action->doneTransforms = action->beforeTransforms;
    action->doneTransforms[0].transform.origin += Vec2F(50.0f, 0.0f);
    action->Redo();

    Vec2F pivotAfter = a->GetPivot();
    EXPECT_TRUE(NearV(pivotAfter, pivotBefore + Vec2F(50.0f, 0.0f)));
}

namespace
{
    bool ChangedListContains(const Ref<Actor>& a)
    {
        for (auto& o : o2Scene.GetChangedObjects())
            if (o && o->GetID() == a->GetID())
                return true;
        return false;
    }
}

TEST(TransformAction, RedoMarksObjectAsChanged)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    auto action = mmake<TransformAction>(AsEditable({a}));
    action->doneTransforms = action->beforeTransforms;
    action->doneTransforms[0].transform.origin += Vec2F(50.0f, 0.0f);
    action->Redo();

    EXPECT_TRUE(ChangedListContains(a));
}

TEST(TransformAction, UndoMarksObjectAsChanged)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<TransformAction>(AsEditable({a}));
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();
    o2Scene.CheckChangedObjects();
    a->changedFrame = -1;

    action->Undo();

    EXPECT_TRUE(ChangedListContains(a));
}

TEST(TransformAction, AppendStepChainKeepsWorldPivotConsistent)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetSize(Vec2F(100.0f, 100.0f));
    TickScene();

    auto editable = AsEditable({a});

    Vec2F pivot = a->GetPivot();
    auto main = mmake<TransformAction>(editable);

    for (int i = 0; i < 5; i++)
    {
        auto step = mmake<TransformAction>(editable);
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].transform.origin += Vec2F(10.0f, 0.0f);
        main->Append(step);

        pivot += Vec2F(10.0f, 0.0f);
        EXPECT_TRUE(NearV(a->GetPivot(), pivot)) << "step " << i;
    }
}
