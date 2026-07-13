#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Tools/ScaleTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct MoveDragProbe: MoveTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void UpdateHandles() { UpdateHandlesPosition(); }
        void PressAxis(int axis) { Axis3DHandlePressed(axis); }
        void MoveAxis(int axis) { OnAxis3DHandleMoved(axis); }
        const Ref<SceneDragHandle3D>& AxisHandle(int axis) const { return GetAxisHandle3D(axis); }
        const Ref<SceneDragHandle3D>& PlaneHandle(int axis) const { return GetPlaneHandle3D(axis); }
    };

    struct ScaleDragProbe: ScaleTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void UpdateHandles() { UpdateHandles3D(); }
        void PressAxis(int axis) { Axis3DHandlePressed(axis); }
        void MoveAxis(int axis) { OnAxis3DHandleMoved(axis); }
        const Ref<SceneDragHandle3D>& AxisHandle(int axis) const { return GetAxisHandle3D(axis); }
    };

    struct ToolsDragFixture
    {
        SceneCleanGuard guard;

        ToolsDragFixture()
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

        ~ToolsDragFixture()
        {
            o2Input.OnCursorReleased(0);
            o2Input.PreUpdate();
            o2Input.Update(0.016f);

            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({});
            o2EditorSceneScreen.SetView3DMode(false);
        }

        Ref<Actor> MakeSelectedActor(const Vec3F& eulerAngles = Vec3F())
        {
            auto actor = MakeActor(Vec2F(0.0f, 0.0f));
            actor->transform->SetSize2D(Vec2F(100.0f, 60.0f));
            actor->transform->SetEulerAngles(eulerAngles);
            TickScene();
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ actor->GetID() });
            return actor;
        }

        static Vec2F Project(const Vec3F& worldPoint)
        {
            return o2EditorSceneScreen.World3DToScreenPoint(worldPoint);
        }

        static void PressAt(const Vec2F& screenPoint)
        {
            o2Input.OnCursorPressed(screenPoint);
            o2Input.PreUpdate();
        }

        static void MoveTo(const Vec2F& screenPoint)
        {
            o2Input.OnCursorMoved(screenPoint);
            o2Input.PreUpdate();
        }

        static void ReleaseCursor()
        {
            o2Input.OnCursorReleased(0);
            o2Input.PreUpdate();
        }
    };

    bool NearVec3(const Vec3F& a, const Vec3F& b, float eps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }
}

// Move: pressing an axis arrow must not teleport the object, small cursor steps
// must produce matching small position deltas along the axis only
TEST(TransformTools3DDrag, MoveAxisDragIsAnchoredAtPressPoint)
{
    ToolsDragFixture fixture;
    auto actor = fixture.MakeSelectedActor();

    auto probe = mmake<MoveDragProbe>();
    probe->Enable();
    probe->UpdateHandles();

    // Grab the X arrow away from the object center
    Vec3F axisDir(1.0f, 0.0f, 0.0f);
    fixture.PressAt(fixture.Project(axisDir*80.0f));
    probe->PressAxis(0);

    // No jump at press
    EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), Vec3F(), 0.01f))
        << "object must not move at press";

    // Tiny drag: matching tiny delta, not a snap to the grab point
    fixture.MoveTo(fixture.Project(axisDir*82.0f));
    probe->MoveAxis(0);
    EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), axisDir*2.0f, 0.5f))
        << "first drag step must move by the cursor delta, got "
        << actor->transform->GetPosition().x << " " << actor->transform->GetPosition().y
        << " " << actor->transform->GetPosition().z;

    // Long drag along the axis
    fixture.MoveTo(fixture.Project(axisDir*180.0f));
    probe->MoveAxis(0);
    EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), axisDir*100.0f, 1.0f));

    // No release through the tool: completing the action requires the SceneWindow singleton
    fixture.ReleaseCursor();
    probe->Disable();
}

// Move: dragging a pre-rotated object along its local axis must keep the rotation and scale
TEST(TransformTools3DDrag, MoveKeepsOrientationOfPreRotatedObject)
{
    ToolsDragFixture fixture;
    Vec3F eulerAngles(0.3f, 0.2f, 0.4f);
    auto actor = fixture.MakeSelectedActor(eulerAngles);

    auto probe = mmake<MoveDragProbe>();
    probe->Enable();
    probe->UpdateHandles();

    // Local frame: the X handle points along the actor's local X
    Vec3F axisDir = actor->transform->GetRotation()*Vec3F(1.0f, 0.0f, 0.0f);

    fixture.PressAt(fixture.Project(axisDir*80.0f));
    probe->PressAxis(0);

    fixture.MoveTo(fixture.Project(axisDir*180.0f));
    probe->MoveAxis(0);

    EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), axisDir*100.0f, 1.0f))
        << "position must move along the local axis, got "
        << actor->transform->GetPosition().x << " " << actor->transform->GetPosition().y
        << " " << actor->transform->GetPosition().z;

    EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
        << "rotation must not change during move, got "
        << actor->transform->GetEulerAngles().x << " " << actor->transform->GetEulerAngles().y
        << " " << actor->transform->GetEulerAngles().z;

    EXPECT_TRUE(NearVec3(actor->transform->GetScale(), Vec3F(1.0f, 1.0f, 1.0f), 0.001f))
        << "scale must not change during move";

    // No release through the tool: completing the action requires the SceneWindow singleton
    fixture.ReleaseCursor();
    probe->Disable();
}

// Move: the visible arrow point must pick its own handle, not a neighbour
TEST(TransformTools3DDrag, MoveArrowPickingMatchesVisibleGeometry)
{
    ToolsDragFixture fixture;
    fixture.MakeSelectedActor();

    auto probe = mmake<MoveDragProbe>();
    probe->Enable();
    probe->UpdateHandles();

    // Register handles for cursor events like the editor draw pass does
    for (int axis = 0; axis < 3; axis++)
    {
        probe->AxisHandle(axis)->Draw();
        probe->PlaneHandle(axis)->Draw();
    }

    for (int axis = 0; axis < 3; axis++)
    {
        float handleScale = probe->AxisHandle(axis)->GetWorldScale();
        Vec2F arrowPoint = fixture.Project(Vec3F::Axis(axis)*(handleScale*0.6f));

        EXPECT_TRUE(probe->AxisHandle(axis)->IsUnderPoint(arrowPoint))
            << "arrow of axis " << axis << " must be picked at its shaft";

        for (int other = 0; other < 3; other++)
        {
            if (other != axis)
            {
                EXPECT_FALSE(probe->AxisHandle(other)->IsUnderPoint(arrowPoint))
                    << "axis " << other << " must not be picked at axis " << axis << " arrow";
            }
        }
    }

    probe->Disable();
}


// Move: objects rotated to degenerate 2D projections (like upright cylinders with 90 degrees
// pitch) must keep their full orientation when moved
TEST(TransformTools3DDrag, MoveKeepsOrientationAtDegenerateProjections)
{
    Vector<Vec3F> rotations = { Vec3F(Math::Deg2rad(90.0f), 0.0f, 0.5f),
                                Vec3F(0.0f, Math::Deg2rad(90.0f), 0.5f),
                                Vec3F(Math::Deg2rad(90.0f), 0.0f, 0.0f) };

    for (auto& eulerAngles : rotations)
    {
        ToolsDragFixture fixture;
        auto actor = fixture.MakeSelectedActor(eulerAngles);

        auto probe = mmake<MoveDragProbe>();
        probe->Enable();
        probe->UpdateHandles();

        Vec3F axisDir = actor->transform->GetRotation()*Vec3F(1.0f, 0.0f, 0.0f);

        fixture.PressAt(fixture.Project(axisDir*80.0f));
        probe->PressAxis(0);

        fixture.MoveTo(fixture.Project(axisDir*180.0f));
        probe->MoveAxis(0);

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.01f))
            << "rotation (" << eulerAngles.x << " " << eulerAngles.y << " " << eulerAngles.z
            << ") must survive the move, got "
            << actor->transform->GetEulerAngles().x << " " << actor->transform->GetEulerAngles().y
            << " " << actor->transform->GetEulerAngles().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), axisDir*100.0f, 1.5f))
            << "position must move along the local axis";

        fixture.ReleaseCursor();
        probe->Disable();
    }
}

// Scale: dragging each axis arrow must scale its own component only,
// keeping position and rotation
TEST(TransformTools3DDrag, ScaleAxisDragScalesOwnComponent)
{
    for (int axis = 0; axis < 3; axis++)
    {
        ToolsDragFixture fixture;
        auto actor = fixture.MakeSelectedActor();

        auto probe = mmake<ScaleDragProbe>();
        probe->Enable();
        probe->UpdateHandles();

        Vec3F axisDir = Vec3F::Axis(axis);

        fixture.PressAt(fixture.Project(axisDir*100.0f));
        probe->PressAxis(axis);

        fixture.MoveTo(fixture.Project(axisDir*200.0f));
        probe->MoveAxis(axis);

        Vec3F expectedScale(axis == 0 ? 2.0f : 1.0f, axis == 1 ? 2.0f : 1.0f, axis == 2 ? 2.0f : 1.0f);
        EXPECT_TRUE(NearVec3(actor->transform->GetScale(), expectedScale, 0.05f))
            << "axis " << axis << " scale expected (" << expectedScale.x << " " << expectedScale.y
            << " " << expectedScale.z << "), got " << actor->transform->GetScale().x << " "
            << actor->transform->GetScale().y << " " << actor->transform->GetScale().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), Vec3F(), 0.5f))
            << "axis " << axis << ": position must stay while scaling";

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), Vec3F(), 0.001f))
            << "axis " << axis << ": rotation must stay while scaling";

        // No release through the tool: completing the action requires the SceneWindow singleton
        fixture.ReleaseCursor();
        probe->Disable();
    }
}
