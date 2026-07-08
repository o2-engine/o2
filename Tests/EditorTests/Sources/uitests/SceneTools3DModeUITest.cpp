#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Tools/FrameTool.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Tools/RotateTool.h"
#include "o2Editor/Tools/ScaleTool.h"
#include "o2Editor/Tools/SelectionTool.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    // Probes expose protected tool internals for visibility assertions
    struct MoveToolProbe: MoveTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        bool Handles2DEnabled() const
        {
            return mHorDragHandle->IsEnabled() && mVerDragHandle->IsEnabled() && mBothDragHandle->IsEnabled();
        }

        bool Handles3DEnabled() const
        {
            return mXDragHandle3D->IsEnabled() && mYDragHandle3D->IsEnabled() && mZDragHandle3D->IsEnabled();
        }

        bool PlaneHandles3DEnabled() const
        {
            return mXYPlaneHandle3D->IsEnabled() && mXZPlaneHandle3D->IsEnabled() && mYZPlaneHandle3D->IsEnabled();
        }

        const Ref<SceneDragHandle3D>& AxisHandle(int axis) const { return GetAxisHandle3D(axis); }
        const Ref<SceneDragHandle3D>& PlaneHandle(int normalAxis) const { return GetPlaneHandle3D(normalAxis); }

        void PressAxis(int axis) { Axis3DHandlePressed(axis); }
        void MoveAxis(int axis) { OnAxis3DHandleMoved(axis); }

        float ZHandlePositionZ() const { return mZDragHandle3D->GetPositionZ(); }
    };

    struct ScaleToolProbe: ScaleTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        bool Handles2DEnabled() const { return mHorDragHandle->IsEnabled() && mVerDragHandle->IsEnabled(); }
        bool BothHandleEnabled() const { return mBothDragHandle->IsEnabled(); }
        bool UniformHandle3DEnabled() const { return mUniformHandle3D->IsEnabled(); }

        bool Handles3DEnabled() const
        {
            return mXDragHandle3D->IsEnabled() && mYDragHandle3D->IsEnabled() && mZDragHandle3D->IsEnabled();
        }

        bool PlaneHandles3DEnabled() const
        {
            return mXYPlaneHandle3D->IsEnabled() && mXZPlaneHandle3D->IsEnabled() && mYZPlaneHandle3D->IsEnabled();
        }
    };

    struct RotateToolProbe: RotateTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        bool PivotHandleEnabled() const { return mPivotDragHandle->IsEnabled(); }
        const Vector<Vector<Vec2F>>& RingPoints() const { return mRingPoints3D; }
        Vec3F Pivot3D() const { return mPivot3D; }
    };

    struct FrameToolProbe: FrameTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void RefreshFrame() { UpdateSelectionFrame(); }
        void Refresh3D() { Update3DHandles(); }

        bool AnyFrameHandleEnabled() const
        {
            return mLeftTopHandle->IsEnabled() || mRightBottomHandle->IsEnabled() || mPivotHandle->IsEnabled() ||
                mLeftTopRotateHandle->IsEnabled();
        }

        bool Frame3DValid() const { return mFrame3DValid; }
        const o2::AABB& Frame3D() const { return mFrame3D; }
        Quat Frame3DRotation() const { return mFrame3DRotation; }
        Vec3F Corner3D(int corner) const { return GetFrameCorner3D(corner); }
        const Ref<SceneDragHandle3D>& CornerHandle(int corner) const { return mCornerHandles3D[corner]; }
    };

    struct SceneTools3DFixture
    {
        SceneCleanGuard guard;

        SceneTools3DFixture()
        {
            auto& screen = o2EditorSceneScreen;
            *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
            screen.UpdateSelfTransform();
        }

        ~SceneTools3DFixture()
        {
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({});
            o2EditorSceneScreen.SetView3DMode(false);
        }

        Ref<Actor> MakeSelectedActor()
        {
            auto actor = MakeActor(Vec2F(0.0f, 0.0f));
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ actor->GetID() });
            return actor;
        }

        // Enables 3D mode with a deterministic view orientation where all axes project visibly
        void Enable3D()
        {
            o2EditorSceneScreen.SetView3DMode(true);
            o2EditorSceneScreen.GetView3DState().pitch = Math::Deg2rad(30.0f);
            o2EditorSceneScreen.GetView3DState().yaw = 0.0f;
        }
    };
}

TEST(SceneTools3DMode, SwitchingToolsIn3DModeDoesNotCrash)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();

    for (bool mode3D : { true, false, true, false })
    {
        if (mode3D)
            fixture.Enable3D();
        else
            screen.SetView3DMode(false);

        screen.SelectTool<SelectionTool>();
        screen.Update(0.016f);
        screen.SelectTool<MoveTool>();
        screen.Update(0.016f);
        screen.SelectTool<RotateTool>();
        screen.Update(0.016f);
        screen.SelectTool<ScaleTool>();
        screen.Update(0.016f);
        screen.SelectTool<FrameTool>();
        screen.Update(0.016f);
    }

    screen.SelectTool<MoveTool>();
    EXPECT_FALSE(screen.IsView3DMode());
}

TEST(SceneTools3DMode, MoveToolHandleVisibilityMatchesMode)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetPositionZ(50.0f);
    TickScene();

    auto probe = mmake<MoveToolProbe>();
    probe->Enable();
    probe->Tick();

    EXPECT_TRUE(probe->Handles2DEnabled());
    EXPECT_FALSE(probe->Handles3DEnabled());

    fixture.Enable3D();
    probe->Tick();

    EXPECT_FALSE(probe->Handles2DEnabled());
    EXPECT_TRUE(probe->Handles3DEnabled());
    EXPECT_TRUE(probe->PlaneHandles3DEnabled());
    EXPECT_NEAR(probe->ZHandlePositionZ(), 50.0f, 1e-3f) << "3D handles must sit at the selection world position";

    for (int axis = 0; axis < 3; axis++)
    {
        Vec3F position = probe->AxisHandle(axis)->GetPosition3D();
        EXPECT_NEAR(position.z, 50.0f, 1e-3f);

        // Arrow geometry maps local +Y to its world axis
        Vec3F mapped = probe->AxisHandle(axis)->GetRotation3D()*Vec3F::YAxis();
        Vec3F expected = Vec3F::Axis(axis);
        EXPECT_NEAR(mapped.x, expected.x, 1e-3f);
        EXPECT_NEAR(mapped.y, expected.y, 1e-3f);
        EXPECT_NEAR(mapped.z, expected.z, 1e-3f);
    }

    screen.SetView3DMode(false);
    probe->Tick();

    EXPECT_TRUE(probe->Handles2DEnabled());
    EXPECT_FALSE(probe->Handles3DEnabled());
    EXPECT_FALSE(probe->PlaneHandles3DEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, MoveTool3DHandlePickingByViewRay)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<MoveToolProbe>();
    probe->Enable();

    fixture.Enable3D();
    screen.GetView3DState().yaw = 0.5f;
    probe->Tick();

    ASSERT_TRUE(probe->Handles3DEnabled());

    float distance = screen.GetView3DState().distance;
    float hitDistance = 0.0f;

    // Point over the middle of the X arrow shaft hits only the X arrow
    Vec2F overXArrow = screen.World3DToScreenPoint(Vec3F(distance*0.25f*0.5f, 0.0f, 0.0f));
    EXPECT_TRUE(probe->AxisHandle(0)->GetRayHitDistance(overXArrow, hitDistance));
    EXPECT_FALSE(probe->AxisHandle(1)->GetRayHitDistance(overXArrow, hitDistance));
    EXPECT_FALSE(probe->AxisHandle(2)->GetRayHitDistance(overXArrow, hitDistance));

    // Point over the middle of the XY plane quad hits the plane handle and no arrows
    float quadCenter = distance*0.25f*0.2f;
    Vec2F overXYQuad = screen.World3DToScreenPoint(Vec3F(quadCenter, quadCenter, 0.0f));
    EXPECT_TRUE(probe->PlaneHandle(2)->GetRayHitDistance(overXYQuad, hitDistance));
    EXPECT_FALSE(probe->AxisHandle(0)->GetRayHitDistance(overXYQuad, hitDistance));

    // Far point hits nothing
    Vec2F farPoint(5.0f, 5.0f);
    for (int axis = 0; axis < 3; axis++)
        EXPECT_FALSE(probe->AxisHandle(axis)->GetRayHitDistance(farPoint, hitDistance));

    probe->Disable();
}

TEST(SceneTools3DMode, MoveTool3DHandlesUseLocalFrame)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetSize2D(Vec2F(100.0f, 60.0f));
    actor->transform->SetAngle(0.5f);
    TickScene();

    auto probe = mmake<MoveToolProbe>();
    probe->Enable();

    fixture.Enable3D();
    screen.GetView3DState().yaw = 0.5f;
    probe->Tick();

    Quat frame = actor->transform->GetRotation();

    // Arrows point along the local axes of the selected actor
    for (int axis = 0; axis < 3; axis++)
    {
        Vec3F mapped = probe->AxisHandle(axis)->GetRotation3D()*Vec3F::YAxis();
        Vec3F expected = frame*Vec3F::Axis(axis);
        EXPECT_NEAR(mapped.x, expected.x, 1e-3f);
        EXPECT_NEAR(mapped.y, expected.y, 1e-3f);
        EXPECT_NEAR(mapped.z, expected.z, 1e-3f);
    }

    // Dragging the local X arrow moves the actor along the world direction of local X
    Vec3F localXDir = frame*Vec3F::XAxis();
    float distance = screen.GetView3DState().distance;

    Vec2F pressPoint = screen.World3DToScreenPoint(localXDir*(distance*0.1f));
    Vec2F movePoint = screen.World3DToScreenPoint(localXDir*(distance*0.2f));

    o2Input.OnCursorPressed(pressPoint);
    o2Input.PreUpdate();
    probe->PressAxis(0);

    o2Input.OnCursorMoved(movePoint);
    o2Input.PreUpdate();
    probe->MoveAxis(0);

    Vec3F position = actor->transform->GetWorldPosition();
    Vec3F expectedPosition = localXDir*(distance*0.1f);

    EXPECT_NEAR(position.x, expectedPosition.x, 0.5f);
    EXPECT_NEAR(position.y, expectedPosition.y, 0.5f);
    EXPECT_NEAR(position.z, expectedPosition.z, 0.5f);

    o2Input.OnCursorReleased(0);
    o2Input.PreUpdate();
    o2Input.Update(0.016f);

    probe->Disable();
}

TEST(SceneTools3DMode, MoveTool3DHandlesRequireSelection)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;

    auto probe = mmake<MoveToolProbe>();
    probe->Enable();

    fixture.Enable3D();
    probe->Tick();

    EXPECT_FALSE(probe->Handles2DEnabled());
    EXPECT_FALSE(probe->Handles3DEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, ScaleToolHandleVisibilityMatchesMode)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<ScaleToolProbe>();
    probe->Enable();
    probe->Tick();

    EXPECT_TRUE(probe->Handles2DEnabled());
    EXPECT_TRUE(probe->BothHandleEnabled());
    EXPECT_FALSE(probe->Handles3DEnabled());

    fixture.Enable3D();
    probe->Tick();

    EXPECT_FALSE(probe->Handles2DEnabled());
    EXPECT_FALSE(probe->BothHandleEnabled()) << "2D center handle is replaced by the volumetric uniform cube in 3D";
    EXPECT_TRUE(probe->UniformHandle3DEnabled());
    EXPECT_TRUE(probe->Handles3DEnabled());
    EXPECT_TRUE(probe->PlaneHandles3DEnabled());

    screen.SetView3DMode(false);
    probe->Tick();

    EXPECT_TRUE(probe->Handles2DEnabled());
    EXPECT_TRUE(probe->BothHandleEnabled());
    EXPECT_FALSE(probe->Handles3DEnabled());
    EXPECT_FALSE(probe->PlaneHandles3DEnabled());
    EXPECT_FALSE(probe->UniformHandle3DEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, RotateToolRingsAndPickingIn3D)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<RotateToolProbe>();
    probe->Enable();
    probe->Tick();
    EXPECT_TRUE(probe->PivotHandleEnabled());

    fixture.Enable3D();
    probe->Tick();
    EXPECT_FALSE(probe->PivotHandleEnabled()) << "2D pivot handle must hide in 3D mode";

    probe->UpdateRings3D();
    ASSERT_EQ(probe->RingPoints().Count(), 3);
    for (auto& ring : probe->RingPoints())
        EXPECT_GT(ring.Count(), 32);

    // The 45 degrees sample of the Z ring lies only on the Z ring, so the pick is unambiguous
    int zSampleIndex = 64/8;
    Vec2F zRingPoint = probe->RingPoints()[2][zSampleIndex];
    EXPECT_EQ(probe->PickRing3D(zRingPoint), 2);

    float angle = 0.0f;
    ASSERT_TRUE(probe->GetCursorRingAngle3D(2, zRingPoint, angle));
    EXPECT_NEAR(angle, Math::PI()*0.25f, 0.05f);

    // Far away point picks nothing
    EXPECT_EQ(probe->PickRing3D(Vec2F(-1000.0f, -1000.0f)), -1);

    screen.SetView3DMode(false);
    probe->Tick();
    EXPECT_TRUE(probe->PivotHandleEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, FrameToolHandlesHiddenIn3D)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<FrameToolProbe>();
    probe->Enable();
    probe->RefreshFrame();
    EXPECT_TRUE(probe->AnyFrameHandleEnabled());

    fixture.Enable3D();
    probe->RefreshFrame();
    EXPECT_FALSE(probe->AnyFrameHandleEnabled()) << "2D frame handles are meaningless as projected in 3D mode";

    screen.SetView3DMode(false);
    probe->RefreshFrame();
    EXPECT_TRUE(probe->AnyFrameHandleEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, FrameTool3DBoundsHandles)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetSize2D(Vec2F(100.0f, 60.0f));
    TickScene();

    auto probe = mmake<FrameToolProbe>();
    probe->Enable();

    fixture.Enable3D();
    probe->Refresh3D();

    ASSERT_TRUE(probe->Frame3DValid());

    // The frame bounds the flat actor rect
    Vec3F size = probe->Frame3D().GetSize();
    EXPECT_NEAR(size.x, 100.0f, 0.5f);
    EXPECT_NEAR(size.y, 60.0f, 0.5f);
    EXPECT_NEAR(size.z, 0.0f, 1e-3f);

    for (int corner = 0; corner < 8; corner++)
    {
        EXPECT_TRUE(probe->CornerHandle(corner)->IsEnabled());

        Vec3F position = probe->CornerHandle(corner)->GetPosition3D();
        Vec3F expected = probe->Corner3D(corner);
        EXPECT_NEAR(position.x, expected.x, 1e-3f);
        EXPECT_NEAR(position.y, expected.y, 1e-3f);
        EXPECT_NEAR(position.z, expected.z, 1e-3f);

        // Bracket arms point inwards along the frame edges
        Vec3F armX = probe->CornerHandle(corner)->GetRotation3D()*Vec3F::XAxis();
        Vec3F armY = probe->CornerHandle(corner)->GetRotation3D()*Vec3F::YAxis();

        float inwardsX = (corner & 1) ? -1.0f : 1.0f;
        float inwardsY = (corner & 2) ? -1.0f : 1.0f;

        EXPECT_GT(armX.x*inwardsX + armX.y*inwardsY, 0.9f);
        EXPECT_GT(armY.x*inwardsX + armY.y*inwardsY, 0.9f);
    }

    screen.SetView3DMode(false);
    probe->Refresh3D();

    EXPECT_FALSE(probe->Frame3DValid());
    for (int corner = 0; corner < 8; corner++)
        EXPECT_FALSE(probe->CornerHandle(corner)->IsEnabled());

    probe->Disable();
}

TEST(SceneTools3DMode, FrameToolSingleSelectionUsesOrientedLocalBox)
{
    SceneTools3DFixture fixture;
    auto& screen = o2EditorSceneScreen;
    auto actor = fixture.MakeSelectedActor();
    actor->transform->SetSize2D(Vec2F(100.0f, 60.0f));
    actor->transform->SetAngle(0.5f);
    TickScene();

    auto probe = mmake<FrameToolProbe>();
    probe->Enable();

    fixture.Enable3D();
    probe->Refresh3D();

    ASSERT_TRUE(probe->Frame3DValid());

    // The oriented box keeps the tight local size instead of the inflated world AABB
    Vec3F size = probe->Frame3D().GetSize();
    EXPECT_NEAR(size.x, 100.0f, 0.5f);
    EXPECT_NEAR(size.y, 60.0f, 0.5f);

    Quat frame = probe->Frame3DRotation();
    EXPECT_GT(Math::Abs(frame.Dot(actor->transform->GetRotation())), 0.9999f);

    // Corners follow the rotated rect
    Vec2F expectedCorner = Vec2F(-50.0f, -30.0f).Rotate(0.5f);
    Vec3F corner = probe->Corner3D(0);
    EXPECT_NEAR(corner.x, expectedCorner.x, 0.5f);
    EXPECT_NEAR(corner.y, expectedCorner.y, 0.5f);

    // Multi selection falls back to the world axis aligned frame
    auto second = MakeActor(Vec2F(200.0f, 0.0f));
    second->transform->SetSize2D(Vec2F(10.0f, 10.0f));
    TickScene();
    screen.SelectObjectsByIdsWithoutAction({ actor->GetID(), second->GetID() });

    probe->Refresh3D();
    ASSERT_TRUE(probe->Frame3DValid());
    EXPECT_GT(Math::Abs(probe->Frame3DRotation().Dot(Quat::Identity())), 0.9999f);

    probe->Disable();
}
