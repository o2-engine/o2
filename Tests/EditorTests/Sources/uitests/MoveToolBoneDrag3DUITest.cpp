#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Events/EventSystem.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct BoneDragProbe: MoveTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        const Ref<SceneDragHandle3D>& AxisHandle(int axis) const { return GetAxisHandle3D(axis); }
        const Ref<SceneDragHandle3D>& PlaneHandle(int axis) const { return GetPlaneHandle3D(axis); }
    };

    // Drives the move tool through the real cursor pipeline like MoveTool3DEventFlowUITest,
    // but over a deeply nested bone-like actor under 3D-rotated parents
    struct BoneDragFixture
    {
        SceneCleanGuard guard;
        Ref<BoneDragProbe> probe;

        BoneDragFixture()
        {
            auto& screen = o2EditorSceneScreen;
            *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
            screen.UpdateSelfTransform();

            screen.SetView3DMode(true);
            screen.GetView3DState().target = Vec3F();
            screen.GetView3DState().distance = 500.0f;
            screen.GetView3DState().pitch = Math::Deg2rad(40.0f);
            screen.GetView3DState().yaw = 0.6f;

            probe = mmake<BoneDragProbe>();
            probe->Enable();
        }

        ~BoneDragFixture()
        {
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

        // Bone hierarchy like a glTF skeleton: zero transform sizes, 3D rotations on every level
        Ref<Actor> MakeSelectedBone()
        {
            auto model = MakeActor(Vec2F());
            model->transform->SetEulerAngles(Vec3F(Math::Deg2rad(-90.0f), 0.0f, Math::Deg2rad(20.0f)));

            auto spine = mmake<Actor>(ActorCreateMode::InScene);
            model->AddChild(spine);
            spine->transform->SetSize(Vec3F());
            spine->transform->SetPosition(Vec3F(0.0f, 10.0f, 0.0f));
            spine->transform->SetEulerAngles(Vec3F(-0.2f, 0.5f, -0.3f));

            auto bone = mmake<Actor>(ActorCreateMode::InScene);
            spine->AddChild(bone);
            bone->transform->SetSize(Vec3F());
            bone->transform->SetPosition(Vec3F(5.0f, 3.0f, 2.0f));
            bone->transform->SetEulerAngles(Vec3F(-0.45f, 0.75f, 0.7f));

            TickScene();
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ bone->GetID() });
            probe->Tick();
            return bone;
        }

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

    Quat WorldRotation(const Ref<Actor>& actor)
    {
        Vec3F position, scale;
        Quat rotation;
        actor->transform->GetWorldTransform3D().Decompose(position, rotation, scale);
        return rotation;
    }
}

// Dragging a nested bone by the move tool arrows must shift it along the gizmo axis by the dragged
// amount - not teleport it: the world delta must be converted to the bone local space through the
// full 3D parent transform, and the bone orientation must survive the drag
TEST(MoveToolBoneDrag3D, DragMovesNestedBoneAlongAxisWithoutJump)
{
    for (int axis = 0; axis < 3; axis++)
    {
        BoneDragFixture fixture;
        auto bone = fixture.MakeSelectedBone();

        const Vec3F boneEuler(-0.45f, 0.75f, 0.7f);
        Vec3F startWorld = bone->transform->GetWorldPosition();
        Vec3F axisDir = WorldRotation(bone)*Vec3F::Axis(axis);
        float handleScale = fixture.probe->AxisHandle(axis)->GetWorldScale();

        Vec2F pressPoint = fixture.Project(startWorld + axisDir*(handleScale*0.6f));

        o2Input.OnCursorMoved(pressPoint);
        fixture.PumpFrame();
        fixture.PressAt(pressPoint);

        ASSERT_TRUE(fixture.probe->AxisHandle(axis)->IsPressed())
            << "axis " << axis << ": the arrow must receive the press";

        EXPECT_TRUE(NearVec3(bone->transform->GetWorldPosition(), startWorld, 0.01f))
            << "axis " << axis << ": bone must not move at press";

        fixture.MoveTo(fixture.Project(startWorld + axisDir*(handleScale*0.6f + 10.0f)));
        fixture.MoveTo(fixture.Project(startWorld + axisDir*(handleScale*0.6f + 50.0f)));

        Vec3F endWorld = bone->transform->GetWorldPosition();
        EXPECT_TRUE(NearVec3(endWorld, startWorld + axisDir*50.0f, 1.5f))
            << "axis " << axis << ": bone must move along the axis by the dragged amount, got world offset "
            << (endWorld - startWorld).x << " " << (endWorld - startWorld).y << " " << (endWorld - startWorld).z;

        EXPECT_TRUE(NearVec3(bone->transform->GetEulerAngles(), boneEuler, 0.001f))
            << "axis " << axis << ": bone orientation must survive the drag, got "
            << bone->transform->GetEulerAngles().x << " " << bone->transform->GetEulerAngles().y
            << " " << bone->transform->GetEulerAngles().z;

        EXPECT_TRUE(NearVec3(bone->transform->GetScale(), Vec3F(1.0f, 1.0f, 1.0f), 0.001f))
            << "axis " << axis << ": bone scale must not change during move";
    }
}
