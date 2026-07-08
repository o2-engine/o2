#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Events/EventSystem.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Geometry.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct MoveEventFlowProbe: MoveTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        const Ref<SceneDragHandle3D>& AxisHandle(int axis) const { return GetAxisHandle3D(axis); }
        const Ref<SceneDragHandle3D>& PlaneHandle(int axis) const { return GetPlaneHandle3D(axis); }
    };

    // Drives the move tool through the real cursor pipeline: input queue -> event system ->
    // DragHandle press/drag mechanics -> tool callbacks, instead of calling the callbacks directly
    struct MoveEventFlowFixture
    {
        SceneCleanGuard guard;
        Ref<MoveEventFlowProbe> probe;

        MoveEventFlowFixture()
        {
            auto& screen = o2EditorSceneScreen;
            *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
            screen.UpdateSelfTransform();

            screen.SetView3DMode(true);
            screen.GetView3DState().target = Vec3F();
            screen.GetView3DState().distance = 500.0f;
            screen.GetView3DState().pitch = Math::Deg2rad(40.0f);
            screen.GetView3DState().yaw = 0.6f;

            probe = mmake<MoveEventFlowProbe>();
            probe->Enable();
        }

        ~MoveEventFlowFixture()
        {
            // Completing the transform action on release requires the SceneWindow singleton,
            // so the release is dispatched with the tool callback detached
            for (int axis = 0; axis < 3; axis++)
            {
                probe->AxisHandle(axis)->onReleased.Clear();
                probe->PlaneHandle(axis)->onReleased.Clear();
            }

            o2Input.OnCursorReleased(0);
            PumpFrame();

            probe->Disable();
            probe = nullptr;

            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({});
            o2EditorSceneScreen.SetView3DMode(false);
        }

        // Mesh actors in real scenes keep the default zero transform size:
        // the mesh geometry has its own size, so nothing ever sets the transform one
        Ref<Actor> MakeSelectedActor(const Vec3F& eulerAngles)
        {
            auto actor = MakeActor(Vec2F(0.0f, 0.0f));
            auto mesh = actor->AddComponent<MeshPrimitiveComponent>();
            mesh->SetPrimitiveType(PrimitiveType3D::Box);
            mesh->SetSize(Vec3F(100.0f, 60.0f, 40.0f));
            actor->transform->SetEulerAngles(eulerAngles);
            TickScene();
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ actor->GetID() });
            probe->Tick();
            return actor;
        }

        // One editor-like frame: apply queued input, register the handles like the scene
        // draw pass does, dispatch cursor events, update the tool
        void PumpFrame()
        {
            o2Input.PreUpdate();

            for (int axis = 0; axis < 3; axis++)
            {
                probe->AxisHandle(axis)->Draw();
                probe->PlaneHandle(axis)->Draw();
            }

            o2Events.Update();
            probe->Tick();
            TickScene();
            o2Events.PostUpdate();
            o2Input.Update(0.016f);
        }

        void PressAt(const Vec2F& screenPoint)
        {
            o2Input.OnCursorPressed(screenPoint);
            PumpFrame();
        }

        void MoveTo(const Vec2F& screenPoint)
        {
            o2Input.OnCursorMoved(screenPoint);
            PumpFrame();
        }

        static Vec2F Project(const Vec3F& worldPoint)
        {
            return o2EditorSceneScreen.World3DToScreenPoint(worldPoint);
        }
    };

    bool NearVec3(const Vec3F& a, const Vec3F& b, float eps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }
}

// Move in 3D through the real event flow: grabbing any axis arrow of a rotated object
// must not touch its orientation - not at press and not while dragging
TEST(MoveTool3DEventFlow, DragKeepsOrientationOfRotatedObject)
{
    for (int axis = 0; axis < 3; axis++)
    {
        MoveEventFlowFixture fixture;
        Vec3F eulerAngles(0.4f, 0.3f, 0.6f);
        auto actor = fixture.MakeSelectedActor(eulerAngles);

        Vec3F axisDir = actor->transform->GetRotation()*Vec3F::Axis(axis);
        float handleScale = fixture.probe->AxisHandle(axis)->GetWorldScale();

        // Grab the arrow shaft where it is actually drawn
        Vec2F pressPoint = fixture.Project(axisDir*(handleScale*0.6f));

        // Hover first like a real user: cursor reaches the arrow before the press
        o2Input.OnCursorMoved(pressPoint);
        fixture.PumpFrame();

        fixture.PressAt(pressPoint);

        ASSERT_TRUE(fixture.probe->AxisHandle(axis)->IsPressed())
            << "axis " << axis << ": the arrow must receive the press";

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
            << "axis " << axis << ": rotation must survive the press, got "
            << actor->transform->GetEulerAngles().x << " " << actor->transform->GetEulerAngles().y
            << " " << actor->transform->GetEulerAngles().z;
        EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), Vec3F(), 0.01f))
            << "axis " << axis << ": object must not move at press";

        // Cross the drag threshold and make a visible drag along the arrow
        fixture.MoveTo(fixture.Project(axisDir*(handleScale*0.6f + 10.0f)));
        fixture.MoveTo(fixture.Project(axisDir*(handleScale*0.6f + 50.0f)));

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
            << "axis " << axis << ": rotation must survive the drag, got "
            << actor->transform->GetEulerAngles().x << " " << actor->transform->GetEulerAngles().y
            << " " << actor->transform->GetEulerAngles().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), axisDir*50.0f, 1.5f))
            << "axis " << axis << ": position must move along the local axis, got "
            << actor->transform->GetPosition().x << " " << actor->transform->GetPosition().y
            << " " << actor->transform->GetPosition().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetScale(), Vec3F(1.0f, 1.0f, 1.0f), 0.001f))
            << "axis " << axis << ": scale must not change during move";
    }
}

// Same through the plane handles: dragging a rotated object by any plane quad
// must keep its orientation
TEST(MoveTool3DEventFlow, PlaneDragKeepsOrientationOfRotatedObject)
{
    for (int normalAxis = 0; normalAxis < 3; normalAxis++)
    {
        MoveEventFlowFixture fixture;
        Vec3F eulerAngles(0.4f, 0.3f, 0.6f);
        auto actor = fixture.MakeSelectedActor(eulerAngles);

        Quat rotation = actor->transform->GetRotation();
        float handleScale = fixture.probe->PlaneHandle(normalAxis)->GetWorldScale();

        Vec3F u, v;
        Geometry::AxisPlaneBasis(normalAxis, u, v);
        Vec3F dragDir = rotation*u;

        // A user clicks a visible spot of the quad: scan it for a point where this
        // quad is the topmost handle under the cursor
        for (int i = 0; i < 3; i++)
        {
            fixture.probe->AxisHandle(i)->Draw();
            fixture.probe->PlaneHandle(i)->Draw();
        }

        Vec3F quadPoint;
        bool quadPointFound = false;
        for (float uParam = 0.1f; uParam < 0.31f && !quadPointFound; uParam += 0.05f)
        {
            for (float vParam = 0.1f; vParam < 0.31f && !quadPointFound; vParam += 0.05f)
            {
                Vec3F candidate = rotation*((u*uParam + v*vParam)*handleScale);
                if (fixture.probe->PlaneHandle(normalAxis)->IsUnderPoint(fixture.Project(candidate)))
                {
                    quadPoint = candidate;
                    quadPointFound = true;
                }
            }
        }

        ASSERT_TRUE(quadPointFound) << "plane " << normalAxis << ": no visible quad point to grab";

        o2Input.OnCursorMoved(fixture.Project(quadPoint));
        fixture.PumpFrame();
        fixture.PressAt(fixture.Project(quadPoint));

        ASSERT_TRUE(fixture.probe->PlaneHandle(normalAxis)->IsPressed())
            << "plane " << normalAxis << ": the quad must receive the press";

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
            << "plane " << normalAxis << ": rotation must survive the press";

        fixture.MoveTo(fixture.Project(quadPoint + dragDir*10.0f));
        fixture.MoveTo(fixture.Project(quadPoint + dragDir*50.0f));

        EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
            << "plane " << normalAxis << ": rotation must survive the drag, got "
            << actor->transform->GetEulerAngles().x << " " << actor->transform->GetEulerAngles().y
            << " " << actor->transform->GetEulerAngles().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetPosition(), dragDir*50.0f, 1.5f))
            << "plane " << normalAxis << ": position must move along the plane, got "
            << actor->transform->GetPosition().x << " " << actor->transform->GetPosition().y
            << " " << actor->transform->GetPosition().z;

        EXPECT_TRUE(NearVec3(actor->transform->GetScale(), Vec3F(1.0f, 1.0f, 1.0f), 0.001f))
            << "plane " << normalAxis << ": scale must not change during move";
    }
}
