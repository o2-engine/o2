#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Components/SkinningMeshBoneComponent.h"
#include "o2Editor/Actions/VertexWeights.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<SkinningMeshBoneComponent> AttachBone(const Ref<Actor>& a)
    {
        auto bone = mmake<SkinningMeshBoneComponent>();
        a->AddComponent(bone);
        TickScene();
        return bone;
    }

    bool WeightsEqual(const Vector<Pair<int, float>>& a, const Vector<Pair<int, float>>& b)
    {
        if (a.Count() != b.Count())
            return false;
        for (int i = 0; i < a.Count(); i++)
        {
            if (a[i].first != b[i].first)
                return false;
            if (Math::Abs(a[i].second - b[i].second) > 1e-5f)
                return false;
        }
        return true;
    }
}

TEST(VertexWeightsAction, CtorCapturesBeforeWeights)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto bone = AttachBone(a);
    bone->vertexWeights = { {0, 0.5f}, {1, 0.8f} };

    auto action = mmake<VertexWeightsAction>(bone);

    EXPECT_EQ(action->actorId, a->GetID());
    EXPECT_TRUE(WeightsEqual(action->beforeWeights, { {0, 0.5f}, {1, 0.8f} }));
    EXPECT_FALSE(action->doneCaptured);
}

TEST(VertexWeightsAction, RedoUndoRestoresFullWeightsSnapshot)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto bone = AttachBone(a);
    bone->vertexWeights = { {0, 0.2f} };

    auto action = mmake<VertexWeightsAction>(bone);
    bone->vertexWeights = { {0, 0.7f}, {1, 0.3f} };
    action->Completed();

    bone->vertexWeights = { {99, 9.9f} };

    action->Redo();
    EXPECT_TRUE(WeightsEqual(bone->vertexWeights, { {0, 0.7f}, {1, 0.3f} }));

    action->Undo();
    EXPECT_TRUE(WeightsEqual(bone->vertexWeights, { {0, 0.2f} }));
}

TEST(VertexWeightsAction, AppendCoalescesBrushTickStepsIntoOneAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto bone = AttachBone(a);
    bone->vertexWeights = {};

    auto main = mmake<VertexWeightsAction>(bone);

    {
        auto step = mmake<VertexWeightsAction>(bone);
        step->doneWeights = { {0, 0.1f} };
        step->doneCaptured = true;
        main->Append(step);
    }
    {
        auto step = mmake<VertexWeightsAction>(bone);
        step->doneWeights = { {0, 0.5f} };
        step->doneCaptured = true;
        main->Append(step);
    }
    {
        auto step = mmake<VertexWeightsAction>(bone);
        step->doneWeights = { {0, 0.9f}, {1, 0.4f} };
        step->doneCaptured = true;
        main->Append(step);
    }

    EXPECT_TRUE(WeightsEqual(bone->vertexWeights, { {0, 0.9f}, {1, 0.4f} }));

    main->Undo();
    EXPECT_TRUE(WeightsEqual(bone->vertexWeights, {}));

    main->Redo();
    EXPECT_TRUE(WeightsEqual(bone->vertexWeights, { {0, 0.9f}, {1, 0.4f} }));
}

TEST(VertexWeightsAction, TryMergeRejectsForeignBone)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto boneA = AttachBone(a);
    auto boneB = AttachBone(b);

    auto main = mmake<VertexWeightsAction>(boneA);
    auto step = mmake<VertexWeightsAction>(boneB);
    step->Completed();

    EXPECT_FALSE(main->TryMerge(step));
}

TEST(VertexWeightsAction, TryMergeRejectsStepWithoutDoneCaptured)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto bone = AttachBone(a);

    auto main = mmake<VertexWeightsAction>(bone);
    auto step = mmake<VertexWeightsAction>(bone);

    EXPECT_FALSE(main->TryMerge(step));
}
