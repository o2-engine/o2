#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/BoneTransform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    bool NearB(const Basis& a, const Basis& b, float eps = 1e-3f)
    {
        return NearV(a.origin, b.origin, eps) && NearV(a.xv, b.xv, eps) && NearV(a.yv, b.yv, eps);
    }
}

TEST(BoneTransformAction, CtorCapturesBeforeWorldState)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 20.0f));

    Basis expectedBasis = a->transform->worldBasis;
    Vec2F expectedPos = a->transform->GetWorldPosition();

    auto action = mmake<BoneTransformAction>(a);

    EXPECT_EQ(action->actorId, a->GetID());
    EXPECT_TRUE(NearB(action->beforeWorldBasis, expectedBasis));
    EXPECT_TRUE(NearV(action->beforeWorldPosition, expectedPos));
    EXPECT_FALSE(action->doneCaptured);
}

TEST(BoneTransformAction, CompletedCapturesDoneWorldState)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto action = mmake<BoneTransformAction>(a);
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    Basis expectedBasis = a->transform->worldBasis;
    Vec2F expectedPos = a->transform->GetWorldPosition();
    action->Completed();

    EXPECT_TRUE(action->doneCaptured);
    EXPECT_TRUE(NearB(action->doneWorldBasis, expectedBasis));
    EXPECT_TRUE(NearV(action->doneWorldPosition, expectedPos));
}

TEST(BoneTransformAction, RedoUndoRestoreAndReplay)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));

    auto action = mmake<BoneTransformAction>(a);
    SetActorPos(a, Vec2F(50.0f, 0.0f));
    action->Completed();

    SetActorPos(a, Vec2F(999.0f, 999.0f));

    action->Redo();
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), Vec2F(50.0f, 0.0f)));

    action->Undo();
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), Vec2F(10.0f, 0.0f)));
}

TEST(BoneTransformAction, AppendCoalescesStepsAndAppliesLatest)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    Vec2F initialPos = a->transform->GetWorldPosition();
    auto main = mmake<BoneTransformAction>(a);

    SetActorPos(a, Vec2F(10.0f, 0.0f));
    auto step1 = mmake<BoneTransformAction>(a);
    step1->Completed();
    main->Append(step1);
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), Vec2F(10.0f, 0.0f)));

    SetActorPos(a, Vec2F(25.0f, 0.0f));
    auto step2 = mmake<BoneTransformAction>(a);
    step2->Completed();
    main->Append(step2);
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), Vec2F(25.0f, 0.0f)));

    SetActorPos(a, Vec2F(-1.0f, -1.0f));
    main->Undo();
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), initialPos));

    main->Redo();
    EXPECT_TRUE(NearV(a->transform->GetWorldPosition(), Vec2F(25.0f, 0.0f)));
}

TEST(BoneTransformAction, TryMergeRejectsForeignBone)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    auto main = mmake<BoneTransformAction>(a);
    auto step = mmake<BoneTransformAction>(b);
    step->Completed();

    EXPECT_FALSE(main->TryMerge(step));
}

TEST(BoneTransformAction, TryMergeRejectsStepWithoutDoneCaptured)
{
    SceneCleanGuard guard;
    auto a = MakeActor();

    auto main = mmake<BoneTransformAction>(a);
    auto step = mmake<BoneTransformAction>(a);

    EXPECT_FALSE(main->TryMerge(step));
}
