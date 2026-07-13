#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(TransformAction3D, CtorCaptures3DData)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 20.0f));
    a->transform->SetPositionZ(5.0f);
    a->transform->SetEulerAngles(Vec3F(0.2f, 0.3f, a->transform->GetEulerAngles().z));
    a->transform->SetScaleZ(2.0f);
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({a}));

    ASSERT_EQ(action->beforeTransforms.Count(), 1);
    EXPECT_TRUE(action->beforeTransforms[0].has3D);
    EXPECT_NEAR(action->beforeTransforms[0].positionZ, 5.0f, 1e-4f);
    EXPECT_NEAR(action->beforeTransforms[0].eulerAnglesXY.x, 0.2f, 1e-4f);
    EXPECT_NEAR(action->beforeTransforms[0].eulerAnglesXY.y, 0.3f, 1e-4f);
    EXPECT_NEAR(action->beforeTransforms[0].scaleZ, 2.0f, 1e-4f);
}

TEST(TransformAction3D, UndoRedoRestorePositionAndPositionZ)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    a->transform->SetPositionZ(1.0f);
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({a}));

    SetActorPos(a, Vec2F(50.0f, 0.0f));
    a->transform->SetPositionZ(7.0f);
    TickScene();
    action->Completed();

    SetActorPos(a, Vec2F(999.0f, 999.0f));
    a->transform->SetPositionZ(99.0f);
    TickScene();

    action->Undo();
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(10.0f, 0.0f)));
    EXPECT_NEAR(a->transform->GetPositionZ(), 1.0f, 1e-4f);

    action->Redo();
    EXPECT_TRUE(NearV(a->transform->GetPosition2D(), Vec2F(50.0f, 0.0f)));
    EXPECT_NEAR(a->transform->GetPositionZ(), 7.0f, 1e-4f);
}

TEST(TransformAction3D, UndoRestoresEulerXYAndScaleZ)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetEulerAngles(Vec3F(0.1f, -0.2f, 0.0f));
    a->transform->SetScaleZ(1.5f);
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({a}));

    a->transform->SetEulerAngles(Vec3F(0.6f, 0.7f, 0.0f));
    a->transform->SetScaleZ(3.0f);
    TickScene();
    action->Completed();

    action->Undo();
    Vec3F euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.1f, 1e-4f);
    EXPECT_NEAR(euler.y, -0.2f, 1e-4f);
    EXPECT_NEAR(a->transform->GetScaleZ(), 1.5f, 1e-4f);

    action->Redo();
    euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.6f, 1e-4f);
    EXPECT_NEAR(euler.y, 0.7f, 1e-4f);
    EXPECT_NEAR(a->transform->GetScaleZ(), 3.0f, 1e-4f);
}

TEST(TransformAction3D, RestoreKeeps2DAngleFromBasis)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetSize2D(Vec2F(100.0f, 100.0f));
    a->transform->SetAngle(0.5f);
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({a}));

    a->transform->SetAngle(1.2f);
    a->transform->SetPositionZ(4.0f);
    TickScene();
    action->Completed();

    action->Undo();
    EXPECT_NEAR(a->transform->GetAngle(), 0.5f, 1e-3f);
    EXPECT_NEAR(a->transform->GetPositionZ(), 0.0f, 1e-4f);

    action->Redo();
    EXPECT_NEAR(a->transform->GetAngle(), 1.2f, 1e-3f);
    EXPECT_NEAR(a->transform->GetPositionZ(), 4.0f, 1e-4f);
}

TEST(TransformAction3D, AppendStepRotatesEulerThroughAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetEulerAngles(Vec3F(0.1f, 0.2f, 0.0f));
    TickScene();

    auto main = mmake<TransformAction>(AsEditable({a}));

    // Mirror RotateTool::AppendRotateEulerStep: the mutation is owned by the action
    auto step = mmake<TransformAction>(AsEditable({a}));
    step->doneTransforms = step->beforeTransforms;
    for (auto& t : step->doneTransforms)
    {
        t.eulerAnglesXY.x += 0.3f;
        t.eulerAnglesXY.y -= 0.1f;
    }

    main->Append(step);
    Vec3F euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.4f, 1e-4f);
    EXPECT_NEAR(euler.y, 0.1f, 1e-4f);

    main->Undo();
    euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.1f, 1e-4f);
    EXPECT_NEAR(euler.y, 0.2f, 1e-4f);

    main->Redo();
    euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.4f, 1e-4f);
    EXPECT_NEAR(euler.y, 0.1f, 1e-4f);
}

TEST(TransformAction3D, AppendStepScalesZThroughAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetScaleZ(2.0f);
    TickScene();

    auto main = mmake<TransformAction>(AsEditable({a}));

    // Mirror ScaleTool::AppendScaleStep3D: the mutation is owned by the action
    auto step = mmake<TransformAction>(AsEditable({a}));
    step->doneTransforms = step->beforeTransforms;
    for (auto& t : step->doneTransforms)
        t.scaleZ *= 1.5f;

    main->Append(step);
    EXPECT_NEAR(a->transform->GetScaleZ(), 3.0f, 1e-4f);

    main->Undo();
    EXPECT_NEAR(a->transform->GetScaleZ(), 2.0f, 1e-4f);

    main->Redo();
    EXPECT_NEAR(a->transform->GetScaleZ(), 3.0f, 1e-4f);
}

TEST(TransformAction3D, UndoRedoRestoreSizeZ)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetSizeZ(10.0f);
    TickScene();

    auto action = mmake<TransformAction>(AsEditable({a}));
    EXPECT_NEAR(action->beforeTransforms[0].sizeZ, 10.0f, 1e-4f);

    a->transform->SetSizeZ(25.0f);
    TickScene();
    action->Completed();

    action->Undo();
    EXPECT_NEAR(a->transform->GetSizeZ(), 10.0f, 1e-4f);

    action->Redo();
    EXPECT_NEAR(a->transform->GetSizeZ(), 25.0f, 1e-4f);
}

TEST(TransformAction3D, AppendStepScalesSizeZAroundAnchorThroughAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    a->transform->SetPositionZ(10.0f);
    a->transform->SetSizeZ(4.0f);
    TickScene();

    auto main = mmake<TransformAction>(AsEditable({a}));

    // Mirror FrameTool::AppendScaleAroundAnchorStep3D z path: the mutation is owned by the action
    const float anchorZ = 5.0f, scaleZ = 2.0f;
    auto step = mmake<TransformAction>(AsEditable({a}));
    step->doneTransforms = step->beforeTransforms;
    for (auto& t : step->doneTransforms)
    {
        t.positionZ = anchorZ + (t.positionZ - anchorZ)*scaleZ;
        t.sizeZ *= scaleZ;
    }

    main->Append(step);
    EXPECT_NEAR(a->transform->GetPositionZ(), 15.0f, 1e-4f);
    EXPECT_NEAR(a->transform->GetSizeZ(), 8.0f, 1e-4f);

    main->Undo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 10.0f, 1e-4f);
    EXPECT_NEAR(a->transform->GetSizeZ(), 4.0f, 1e-4f);

    main->Redo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 15.0f, 1e-4f);
    EXPECT_NEAR(a->transform->GetSizeZ(), 8.0f, 1e-4f);
}

TEST(TransformAction3D, AppendStepMovesPositionZThroughAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({a}));

    // Mirror MoveTool::AppendMoveZStep: the mutation is owned by the action
    auto step = mmake<TransformAction>(AsEditable({a}));
    step->doneTransforms = step->beforeTransforms;
    for (auto& t : step->doneTransforms)
        t.positionZ += 10.0f;

    main->Append(step);
    EXPECT_NEAR(a->transform->GetPositionZ(), 10.0f, 1e-4f);

    main->Undo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 0.0f, 1e-4f);

    main->Redo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 10.0f, 1e-4f);
}

TEST(TransformAction3D, TryMergeAcceptsSameObjectsRejectsOthers)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto b = MakeActor(Vec2F(10.0f, 0.0f));
    TickScene();

    auto main = mmake<TransformAction>(AsEditable({a}));

    auto step = mmake<TransformAction>(AsEditable({a}));
    step->doneTransforms = step->beforeTransforms;
    step->doneTransforms[0].positionZ += 5.0f;

    EXPECT_TRUE(main->TryMerge(step));
    ASSERT_EQ(main->doneTransforms.Count(), 1);
    EXPECT_NEAR(main->doneTransforms[0].positionZ, 5.0f, 1e-4f);

    auto foreign = mmake<TransformAction>(AsEditable({b}));
    foreign->doneTransforms = foreign->beforeTransforms;
    EXPECT_FALSE(main->TryMerge(foreign));

    // Merged action keeps the original before state: undo returns to it
    main->Undo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 0.0f, 1e-4f);

    main->Redo();
    EXPECT_NEAR(a->transform->GetPositionZ(), 5.0f, 1e-4f);
}
