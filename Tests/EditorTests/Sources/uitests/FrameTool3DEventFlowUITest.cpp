#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Events/EventSystem.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Tools/FrameTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct FrameEventFlowProbe: FrameTool
    {
        void Enable() { OnEnabled(); }
        void Disable() { OnDisabled(); }
        void Tick() { Update(0.016f); }

        const Ref<SceneDragHandle3D>& CornerHandle(int corner) const { return mCornerHandles3D[corner]; }
        Vec3F Corner3D(int corner) const { return GetFrameCorner3D(corner); }
        bool FrameValid() const { return mFrame3DValid; }
        Quat FrameRotation() const { return mFrame3DRotation; }
        int DragPlaneAxis() const { return mDragPlaneAxis3D; }
    };

    // Drives the frame tool corner handles through the real cursor pipeline: input queue ->
    // event system -> DragHandle press/drag mechanics -> tool callbacks
    struct FrameEventFlowFixture
    {
        SceneCleanGuard guard;
        Ref<FrameEventFlowProbe> probe;

        FrameEventFlowFixture()
        {
            auto& screen = o2EditorSceneScreen;
            *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
            screen.UpdateSelfTransform();

            screen.SetView3DMode(true);
            screen.GetView3DState().target = Vec3F();
            screen.GetView3DState().distance = 500.0f;
            screen.GetView3DState().pitch = Math::Deg2rad(40.0f);
            screen.GetView3DState().yaw = 0.6f;

            probe = mmake<FrameEventFlowProbe>();
            probe->Enable();
        }

        ~FrameEventFlowFixture()
        {
            // Completing the transform action on release requires the SceneWindow singleton,
            // so the release is dispatched with the tool callback detached
            for (int corner = 0; corner < 8; corner++)
                probe->CornerHandle(corner)->onReleased.Clear();

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

        // One editor-like frame: apply queued input, register the corner handles like the
        // scene draw pass does, dispatch cursor events, update the tool
        void PumpFrame()
        {
            o2Input.PreUpdate();

            for (int corner = 0; corner < 8; corner++)
                probe->CornerHandle(corner)->Draw();

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

        // Orbits the camera like a user would until the corner bracket is the topmost
        // handle under its projected point
        bool MakeCornerPickable(int corner)
        {
            for (float pitch : { 40.0f, -40.0f })
            {
                for (int quarter = 0; quarter < 4; quarter++)
                {
                    o2EditorSceneScreen.GetView3DState().pitch = Math::Deg2rad(pitch);
                    o2EditorSceneScreen.GetView3DState().yaw = 0.6f + Math::PI()*0.5f*(float)quarter;
                    probe->Tick();

                    for (int i = 0; i < 8; i++)
                        probe->CornerHandle(i)->Draw();

                    if (probe->CornerHandle(corner)->IsUnderPoint(Project(probe->Corner3D(corner))))
                        return true;
                }
            }

            return false;
        }
    };

    bool NearVec3(const Vec3F& a, const Vec3F& b, float eps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }

    // Frame coordinates of the target cursor point: the corner offset from the anchor with
    // the two in-plane axes scaled, staying on the drag plane through the original corner
    Vec3F ScaledCornerTarget(const Vec3F& cornerOffset, int planeAxis, float factor)
    {
        Vec3F target = cornerOffset;
        float* components = &target.x;
        for (int axis = 0; axis < 3; axis++)
        {
            if (axis != planeAxis)
                components[axis] *= factor;
        }

        return target;
    }
}

// Frame tool in 3D through the real event flow: dragging a corner bracket of a mesh object
// (zero transform size, like real scene objects) must scale it around the opposite corner
TEST(FrameTool3DEventFlow, CornerDragScalesMeshObjectAroundOppositeCorner)
{
    Vector<Vec3F> rotations = { Vec3F(), Vec3F(0.4f, 0.3f, 0.6f) };

    for (auto& eulerAngles : rotations)
    {
        for (int corner = 0; corner < 8; corner++)
        {
            FrameEventFlowFixture fixture;
            auto actor = fixture.MakeSelectedActor(eulerAngles);

            ASSERT_TRUE(fixture.probe->FrameValid());
            ASSERT_TRUE(fixture.MakeCornerPickable(corner))
                << "corner " << corner << ": no camera angle makes the bracket pickable";

            Quat frameRotation = fixture.probe->FrameRotation();
            Vec3F cornerPos = fixture.probe->Corner3D(corner);
            Vec3F anchor = fixture.probe->Corner3D(corner ^ 7);
            Vec3F startPosition = actor->transform->GetWorldPosition();

            // Hover first like a real user, then grab the corner bracket
            Vec2F pressPoint = fixture.Project(cornerPos);
            o2Input.OnCursorMoved(pressPoint);
            fixture.PumpFrame();
            fixture.PressAt(pressPoint);

            ASSERT_TRUE(fixture.probe->CornerHandle(corner)->IsPressed())
                << "corner " << corner << ": the bracket must receive the press";

            int planeAxis = fixture.probe->DragPlaneAxis();
            Vec3F cornerOffset = frameRotation.Inverted()*(cornerPos - anchor);

            // Drag the corner to double the two in-plane axes, via an intermediate step
            // to cross the drag threshold
            for (float factor : { 1.3f, 2.0f })
            {
                Vec3F target = anchor + frameRotation*ScaledCornerTarget(cornerOffset, planeAxis, factor);
                fixture.MoveTo(fixture.Project(target));
            }

            Vec3F expectedScale(planeAxis == 0 ? 1.0f : 2.0f,
                                planeAxis == 1 ? 1.0f : 2.0f,
                                planeAxis == 2 ? 1.0f : 2.0f);

            EXPECT_TRUE(NearVec3(actor->transform->GetScale(), expectedScale, 0.05f))
                << "corner " << corner << " euler (" << eulerAngles.x << " " << eulerAngles.y << " "
                << eulerAngles.z << "): corner drag must scale the object, got scale "
                << actor->transform->GetScale().x << " " << actor->transform->GetScale().y
                << " " << actor->transform->GetScale().z;

            EXPECT_TRUE(NearVec3(actor->transform->GetEulerAngles(), eulerAngles, 0.001f))
                << "corner " << corner << ": rotation must survive the resize";

            EXPECT_TRUE(NearVec3(actor->transform->GetSize(), Vec3F(), 0.001f))
                << "corner " << corner << ": transform size must stay zero for mesh objects";

            // The material point at the anchor stays: the object position orbits accordingly
            Vec3F scaledOffset = frameRotation.Inverted()*(startPosition - anchor);
            scaledOffset.x *= expectedScale.x;
            scaledOffset.y *= expectedScale.y;
            scaledOffset.z *= expectedScale.z;
            Vec3F expectedPosition = anchor + frameRotation*scaledOffset;

            EXPECT_TRUE(NearVec3(actor->transform->GetWorldPosition(), expectedPosition, 1.5f))
                << "corner " << corner << ": position must compensate to keep the opposite corner, got "
                << actor->transform->GetWorldPosition().x << " " << actor->transform->GetWorldPosition().y
                << " " << actor->transform->GetWorldPosition().z << ", expected " << expectedPosition.x
                << " " << expectedPosition.y << " " << expectedPosition.z;

            // The opposite frame corner, recomputed from the actual object bounds after
            // the drag, must stay at its world place for any of the 8 corners
            fixture.probe->Tick();
            EXPECT_TRUE(NearVec3(fixture.probe->Corner3D(corner ^ 7), anchor, 1.0f))
                << "corner " << corner << " euler (" << eulerAngles.x << " " << eulerAngles.y << " "
                << eulerAngles.z << "): the opposite frame corner must stay anchored, got "
                << fixture.probe->Corner3D(corner ^ 7).x << " " << fixture.probe->Corner3D(corner ^ 7).y
                << " " << fixture.probe->Corner3D(corner ^ 7).z << ", expected " << anchor.x << " "
                << anchor.y << " " << anchor.z;
        }
    }
}
