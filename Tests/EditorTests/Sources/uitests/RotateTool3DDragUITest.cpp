#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Actions/Transform.h"
#include "o2Editor/Tools/RotateTool.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct RotateToolDragProbe: RotateTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }

        int PressedRing() const { return mPressedRing3D; }
        Vec3F Pivot3D() const { return mPivot3D; }
        Quat RingFrame() const { return mRingFrame3D; }
        float AccumulatedAngle() const { return mAccumulatedRingAngle3D; }
        const Ref<TransformAction>& Action() const { return mTransformAction; }
    };

    struct RotateDragFixture
    {
        SceneCleanGuard guard;

        RotateDragFixture()
        {
            auto& screen = o2EditorSceneScreen;
            *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
            screen.UpdateSelfTransform();

            screen.SetView3DMode(true);
            screen.GetView3DState().target = Vec3F();
            screen.GetView3DState().distance = 500.0f;
            screen.GetView3DState().pitch = Math::Deg2rad(40.0f);
            screen.GetView3DState().yaw = 0.6f;
        }

        ~RotateDragFixture()
        {
            // No release through the tool: completing the action requires the SceneWindow singleton
            o2Input.OnCursorReleased(0);
            o2Input.PreUpdate();
            o2Input.Update(0.016f);

            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({});
            o2EditorSceneScreen.SetView3DMode(false);
        }

        Ref<Actor> MakeSelectedActor()
        {
            auto actor = MakeActor(Vec2F(0.0f, 0.0f));
            actor->transform->SetSize2D(Vec2F(100.0f, 60.0f));
            TickScene();
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ actor->GetID() });
            return actor;
        }

        static Vec3F RingWorldPoint(int axis, float angle, const Vec3F& pivot, const Quat& frame)
        {
            float radius = Math::Max(o2EditorSceneScreen.GetView3DState().distance, 1.0f)*0.15f;

            Vec3F u, v;
            Geometry::AxisPlaneBasis(axis, u, v);
            return pivot + (frame*u)*Math::Cos(angle)*radius + (frame*v)*Math::Sin(angle)*radius;
        }

        static void SetCamera(float yaw, float pitchDegrees)
        {
            o2EditorSceneScreen.GetView3DState().yaw = yaw;
            o2EditorSceneScreen.GetView3DState().pitch = Math::Deg2rad(pitchDegrees);
        }

        // Presses on ring at fromAngle and drags the cursor through the stop angles along the ring;
        // stop points are generated in the press-time drag plane, matching the tool angle measurement
        static void DragRingSteps(const Ref<RotateToolDragProbe>& probe, int axis, float fromAngle,
                                  const Vector<float>& stopAngles)
        {
            probe->UpdateRings3D();
            Vec3F pivot = probe->Pivot3D();
            Quat frame = probe->RingFrame();

            Vec2F pressPoint = o2EditorSceneScreen.World3DToScreenPoint(RingWorldPoint(axis, fromAngle, pivot, frame));

            ASSERT_EQ(probe->PickRing3D(pressPoint), axis) << "press point must pick the intended ring";

            o2Input.OnCursorPressed(pressPoint);
            o2Input.PreUpdate();

            Input::Cursor cursor(pressPoint, 0);
            probe->OnCursorPressed(cursor);
            ASSERT_EQ(probe->PressedRing(), axis);

            Vec2F lastPoint = pressPoint;
            for (float stopAngle : stopAngles)
            {
                Vec2F movePoint = o2EditorSceneScreen.World3DToScreenPoint(
                    RingWorldPoint(axis, stopAngle, pivot, frame));

                o2Input.OnCursorMoved(movePoint);
                o2Input.PreUpdate();

                cursor.position = movePoint;
                cursor.delta = movePoint - lastPoint;
                probe->OnCursorStillDown(cursor);

                lastPoint = movePoint;
            }
        }

        static void DragRing(const Ref<RotateToolDragProbe>& probe, int axis, float fromAngle, float toAngle)
        {
            DragRingSteps(probe, axis, fromAngle, { toAngle });
        }
    };

    bool QuatNear(const Quat& a, const Quat& b, float minDot = 0.9995f)
    {
        return Math::Abs(a.Dot(b)) > minDot;
    }
}

// Regression for the wrong-axis rotation bug: dragging ring K must change only euler component K,
// in the drag direction
TEST(RotateTool3DDrag, XRingChangesOnlyEulerX)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 0, 0.6f, 0.6f + sweep);

    Vec3F euler = actor->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, sweep, 0.02f);
    EXPECT_NEAR(euler.y, 0.0f, 0.01f);
    EXPECT_NEAR(Math::WrapAngle(euler.z), 0.0f, 0.01f);

    probe->Disable();
}

TEST(RotateTool3DDrag, YRingChangesOnlyEulerY)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 1, 0.6f, 0.6f + sweep);

    Vec3F euler = actor->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.0f, 0.01f);
    EXPECT_NEAR(euler.y, sweep, 0.02f);
    EXPECT_NEAR(Math::WrapAngle(euler.z), 0.0f, 0.01f);

    probe->Disable();
}

TEST(RotateTool3DDrag, ZRingChangesOnlyEulerZ)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 2, 0.6f, 0.6f + sweep);

    Vec3F euler = actor->transform->GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.0f, 0.01f);
    EXPECT_NEAR(euler.y, 0.0f, 0.01f);
    EXPECT_NEAR(Math::WrapAngle(euler.z), sweep, 0.02f);

    probe->Disable();
}

// Rings are oriented by the object local frame: dragging ring K rotates around the world
// direction of the local axis K, like the Unity local rotate mode
TEST(RotateTool3DDrag, XRingRotatesAroundLocalXWithPreRotatedObject)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetEulerAngles(Vec3F(0.0f, 0.5f, 0.0f));
    TickScene();

    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 0, 0.6f, 0.6f + sweep);

    Quat expected = Quat::FromAxisAngle(before*Vec3F::XAxis(), sweep)*before;
    Quat actual = actor->transform->GetRotation();

    EXPECT_GT(Math::Abs(actual.Dot(expected)), 0.9995f)
        << "X ring must rotate around the local X axis of the pre-rotated object";

    probe->Disable();
}

TEST(RotateTool3DDrag, YRingRotatesAroundLocalYWithPreRotatedObject)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetEulerAngles(Vec3F(0.4f, 0.0f, 0.0f));
    TickScene();

    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 1, 0.6f, 0.6f + sweep);

    Quat expected = Quat::FromAxisAngle(before*Vec3F::YAxis(), sweep)*before;
    Quat actual = actor->transform->GetRotation();

    EXPECT_GT(Math::Abs(actual.Dot(expected)), 0.9995f)
        << "Y ring must rotate around the local Y axis of the pre-rotated object";

    probe->Disable();
}

// With Ctrl the rings switch to world axes regardless of the object rotation
TEST(RotateTool3DDrag, CtrlSwitchesRingsToWorldAxes)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetEulerAngles(Vec3F(0.0f, 0.5f, 0.0f));
    TickScene();

    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    o2Input.OnKeyPressed(VK_CONTROL);
    o2Input.PreUpdate();

    const float sweep = 0.35f;
    fixture.DragRing(probe, 0, 0.6f, 0.6f + sweep);

    o2Input.OnKeyReleased(VK_CONTROL);
    o2Input.PreUpdate();
    o2Input.Update(0.016f);

    Quat expected = Quat::FromAxisAngle(Vec3F::XAxis(), sweep)*before;
    Quat actual = actor->transform->GetRotation();

    EXPECT_GT(Math::Abs(actual.Dot(expected)), 0.9995f)
        << "with Ctrl the X ring must rotate around world X";

    probe->Disable();
}

// Regression for rings not following the selection while dragging
TEST(RotateTool3DDrag, RingsFollowSelectionDuringDrag)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    fixture.DragRing(probe, 2, 0.6f, 0.7f);
    ASSERT_EQ(probe->PressedRing(), 2);

    SetActorPos(actor, Vec2F(120.0f, 40.0f));
    TickScene();

    probe->UpdateRings3D();
    Vec3F pivot = probe->Pivot3D();
    EXPECT_NEAR(pivot.x, 120.0f, 1e-3f);
    EXPECT_NEAR(pivot.y, 40.0f, 1e-3f);

    probe->Disable();
}

// Every ring from several pre-rotated states must produce exactly a rotation around the world
// direction of its local axis
TEST(RotateTool3DDrag, PreRotatedCombosRotateAroundLocalAxis)
{
    // Includes degenerate 2D projections (90 degrees pitch/yaw), like upright objects in Z-up scenes
    const Vec3F preRotations[] = { Vec3F(0.0f, 0.0f, 0.0f), Vec3F(0.0f, 0.5f, 0.0f),
                                   Vec3F(0.4f, 0.0f, 0.0f), Vec3F(0.3f, 0.4f, 0.6f),
                                   Vec3F(Math::Deg2rad(90.0f), 0.0f, 0.5f),
                                   Vec3F(0.0f, Math::Deg2rad(90.0f), 0.5f) };

    const float sweep = 0.35f;

    for (auto& preRotation : preRotations)
    {
        for (int axis = 0; axis < 3; axis++)
        {
            RotateDragFixture fixture;
            auto actor = fixture.MakeSelectedActor();
            actor->transform->SetEulerAngles(preRotation);
            TickScene();

            Quat before = actor->transform->GetRotation();

            auto probe = mmake<RotateToolDragProbe>();
            probe->Enable();

            fixture.DragRing(probe, axis, 0.6f, 0.6f + sweep);

            Quat expected = Quat::FromAxisAngle(before*Vec3F::Axis(axis), sweep)*before;
            EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), expected))
                << "ring " << axis << " with pre-rotation (" << preRotation.x << ", " << preRotation.y
                << ", " << preRotation.z << ")";

            probe->Disable();
        }
    }
}

// Regression: basis round trips inside the rotate steps must not shrink the actor or add shear
TEST(RotateTool3DDrag, PreRotatedDragKeepsSizeAndShear)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetEulerAngles(Vec3F(0.0f, 0.5f, 0.0f));
    TickScene();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    fixture.DragRingSteps(probe, 0, 0.6f, { 0.7f, 0.8f, 0.9f, 1.0f });

    Vec2F size = actor->transform->GetSize2D();
    EXPECT_NEAR(size.x, 100.0f, 0.01f);
    EXPECT_NEAR(size.y, 60.0f, 0.01f);
    EXPECT_NEAR(actor->transform->GetShear2D(), 0.0f, 1e-3f);

    probe->Disable();
}

TEST(RotateTool3DDrag, NegativeDragGivesNegativeAngle)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = -0.35f;
    fixture.DragRing(probe, 2, 0.9f, 0.9f + sweep);

    EXPECT_NEAR(Math::WrapAngle(actor->transform->GetAngle()), sweep, 0.02f);
    EXPECT_NEAR(probe->AccumulatedAngle(), sweep, 0.02f);

    probe->Disable();
}

// The world rotation direction must not flip with the camera: from behind, from the side and
// nearly top-down the same ring sweep gives the same world rotation
TEST(RotateTool3DDrag, SignConsistentAcrossCameraViews)
{
    const Vec2F cameraPoses[] = { Vec2F(0.6f, 40.0f), Vec2F(0.6f + Math::PI(), 40.0f), Vec2F(0.3f, 75.0f) };

    const float sweep = 0.35f;

    for (auto& pose : cameraPoses)
    {
        RotateDragFixture fixture;
        fixture.SetCamera(pose.x, pose.y);

        auto actor = fixture.MakeSelectedActor();

        auto probe = mmake<RotateToolDragProbe>();
        probe->Enable();

        fixture.DragRing(probe, 2, 0.6f, 0.6f + sweep);

        EXPECT_NEAR(Math::WrapAngle(actor->transform->GetAngle()), sweep, 0.02f)
            << "camera yaw " << pose.x << " pitch " << pose.y;

        probe->Disable();
    }
}

// Multi step drag accumulates over the ±180 degrees wrap and merges into one undoable action
TEST(RotateTool3DDrag, MultiStepAccumulationCrosses180AndUndoes)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    // 4 x 0.9 rad = 3.6 rad total, crossing pi
    fixture.DragRingSteps(probe, 2, 0.6f, { 1.5f, 2.4f, 3.3f, 4.2f });

    const float total = 3.6f;
    EXPECT_NEAR(probe->AccumulatedAngle(), total, 0.03f);

    Quat expected = Quat::FromAxisAngle(Vec3F::ZAxis(), total)*before;
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), expected));

    // All steps merged into the single drag action: undo returns to the initial state
    auto action = probe->Action();
    ASSERT_TRUE(action);
    action->Completed();

    action->Undo();
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), before));

    action->Redo();
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), expected));

    probe->Disable();
}

// Undo and redo of a drag on a pre-rotated object restore the full quaternion and the size
TEST(RotateTool3DDrag, UndoRedoRestoreQuaternionAndSize)
{
    RotateDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetEulerAngles(Vec3F(0.2f, 0.5f, 0.3f));
    TickScene();

    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    fixture.DragRingSteps(probe, 0, 0.6f, { 0.8f, 1.0f });

    Quat after = actor->transform->GetRotation();
    EXPECT_TRUE(QuatNear(after, Quat::FromAxisAngle(before*Vec3F::XAxis(), 0.4f)*before));

    auto action = probe->Action();
    ASSERT_TRUE(action);
    action->Completed();

    action->Undo();
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), before));
    EXPECT_NEAR(actor->transform->GetSize2D().x, 100.0f, 0.01f);
    EXPECT_NEAR(actor->transform->GetSize2D().y, 60.0f, 0.01f);

    action->Redo();
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), after));
    EXPECT_NEAR(actor->transform->GetSize2D().x, 100.0f, 0.01f);
    EXPECT_NEAR(actor->transform->GetSize2D().y, 60.0f, 0.01f);

    probe->Disable();
}

// Z ring on a multi selection rotates every object and orbits positions around the common center
TEST(RotateTool3DDrag, MultiSelectionZRingOrbitsPositions)
{
    RotateDragFixture fixture;

    auto first = MakeActor(Vec2F(100.0f, 0.0f));
    auto second = MakeActor(Vec2F(-100.0f, 0.0f));
    first->transform->SetSize2D(Vec2F(10.0f, 10.0f));
    second->transform->SetSize2D(Vec2F(10.0f, 10.0f));
    TickScene();

    o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ first->GetID(), second->GetID() });

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();

    const float sweep = Math::PI()*0.5f;
    fixture.DragRing(probe, 2, 0.3f, 0.3f + sweep);

    EXPECT_NEAR(first->transform->GetPosition2D().x, 0.0f, 0.5f);
    EXPECT_NEAR(first->transform->GetPosition2D().y, 100.0f, 0.5f);
    EXPECT_NEAR(second->transform->GetPosition2D().x, 0.0f, 0.5f);
    EXPECT_NEAR(second->transform->GetPosition2D().y, -100.0f, 0.5f);

    EXPECT_NEAR(Math::WrapAngle(first->transform->GetAngle()), sweep, 0.02f);
    EXPECT_NEAR(Math::WrapAngle(second->transform->GetAngle()), sweep, 0.02f);

    probe->Disable();
}

// A ring whose plane is nearly parallel to the view must refuse the drag instead of exploding
TEST(RotateTool3DDrag, EdgeOnRingIsNotDraggable)
{
    RotateDragFixture fixture;

    // With zero yaw the view ray is perpendicular to world X: the X ring is seen edge-on
    fixture.SetCamera(0.0f, 40.0f);

    auto actor = fixture.MakeSelectedActor();
    Quat before = actor->transform->GetRotation();

    auto probe = mmake<RotateToolDragProbe>();
    probe->Enable();
    probe->UpdateRings3D();

    Vec2F pressPoint = o2EditorSceneScreen.World3DToScreenPoint(
        RotateDragFixture::RingWorldPoint(0, 0.6f, probe->Pivot3D(), probe->RingFrame()));

    o2Input.OnCursorPressed(pressPoint);
    o2Input.PreUpdate();

    Input::Cursor cursor(pressPoint, 0);
    probe->OnCursorPressed(cursor);

    EXPECT_EQ(probe->PressedRing(), -1) << "edge-on ring must not start a drag";
    EXPECT_TRUE(QuatNear(actor->transform->GetRotation(), before, 0.99999f));

    probe->Disable();
}
