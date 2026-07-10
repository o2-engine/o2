#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    // Zero-size child under a 3D-rotated parent, like a glTF skeleton bone
    Ref<Actor> MakeBoneUnder3DParent()
    {
        auto parent = MakeActor(Vec2F());
        parent->transform->SetEulerAngles(Vec3F(Math::Deg2rad(-90.0f), 0.0f, 0.3f));

        auto bone = mmake<Actor>(ActorCreateMode::InScene);
        parent->AddChild(bone);
        bone->transform->SetSize(Vec3F());
        bone->transform->SetPosition(Vec3F(5.0f, 3.0f, 2.0f));
        bone->transform->SetEulerAngles(Vec3F(0.2f, -0.4f, 0.1f));
        TickScene();

        return bone;
    }

    void ExpectNearVec3(const Vec3F& value, const Vec3F& expected, float eps)
    {
        EXPECT_NEAR(value.x, expected.x, eps);
        EXPECT_NEAR(value.y, expected.y, eps);
        EXPECT_NEAR(value.z, expected.z, eps);
    }
}

// The 2D world basis projection can't recover local position under a 3D-rotated parent,
// so undo/redo must restore the captured local position directly
TEST(TransformActionNested3D, UndoRedoRestoreLocalPositionUnder3DRotatedParent)
{
    SceneCleanGuard guard;
    auto bone = MakeBoneUnder3DParent();

    auto action = mmake<TransformAction>(AsEditable({ bone }));

    bone->transform->SetPosition(Vec3F(8.0f, -1.0f, 4.0f));
    TickScene();
    action->Completed();

    action->Undo();
    ExpectNearVec3(bone->transform->GetPosition(), Vec3F(5.0f, 3.0f, 2.0f), 1e-3f);
    ExpectNearVec3(bone->transform->GetEulerAngles(), Vec3F(0.2f, -0.4f, 0.1f), 1e-3f);

    action->Redo();
    ExpectNearVec3(bone->transform->GetPosition(), Vec3F(8.0f, -1.0f, 4.0f), 1e-3f);
    ExpectNearVec3(bone->transform->GetEulerAngles(), Vec3F(0.2f, -0.4f, 0.1f), 1e-3f);
}

// Applying an unchanged captured state must keep the bone exactly in place
TEST(TransformActionNested3D, IdentityRedoKeepsBoneInPlace)
{
    SceneCleanGuard guard;
    auto bone = MakeBoneUnder3DParent();

    auto action = mmake<TransformAction>(AsEditable({ bone }));
    action->Completed();

    action->Redo();
    TickScene();

    ExpectNearVec3(bone->transform->GetPosition(), Vec3F(5.0f, 3.0f, 2.0f), 1e-3f);
    ExpectNearVec3(bone->transform->GetEulerAngles(), Vec3F(0.2f, -0.4f, 0.1f), 1e-3f);
    ExpectNearVec3(bone->transform->GetScale(), Vec3F(1.0f, 1.0f, 1.0f), 1e-3f);
}
