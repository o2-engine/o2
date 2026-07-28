#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/Physics/DistanceJoint.h"
#include "o2/Scene/SceneDrawableCategory.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Windows/SceneWindow/GizmosPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneGizmos.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Vector<Vec3F> DrawAndCapture(SceneGizmos& gizmos)
    {
        Vector<Vec3F> points;
        gizmos.Draw([&](const Vec3F& point)
                    {
                        points.Add(point);
                        return Vec2F(point.x, point.y);
                    });

        return points;
    }

    struct CreateModeGuard
    {
        ActorCreateMode prevMode;

        CreateModeGuard(): prevMode(Actor::GetDefaultCreationMode())
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
        }

        ~CreateModeGuard() { Actor::SetDefaultCreationMode(prevMode); }
    };

    Ref<Actor> MakeBoxColliderActor()
    {
        auto actor = mmake<Actor>();
        actor->transform->SetPosition2D(Vec2F(0, 0));
        actor->transform->SetSize2D(Vec2F(20, 20));
        actor->AddComponent<BoxCollider>();
        TickScene();

        return actor;
    }
}

TEST(SceneGizmos, OnlyOverridingTypesAreGizmosDrawers)
{
    SceneGizmos gizmos;

    EXPECT_TRUE(gizmos.IsGizmosDrawer(TypeOf(BoxCollider)));
    EXPECT_TRUE(gizmos.IsGizmosDrawer(TypeOf(DistanceJoint))); // inherited from IJoint
    EXPECT_TRUE(gizmos.IsGizmosDrawer(TypeOf(CameraActor)));

    EXPECT_FALSE(gizmos.IsGizmosDrawer(TypeOf(ImageComponent)));
    EXPECT_FALSE(gizmos.IsGizmosDrawer(TypeOf(Actor)));
    EXPECT_FALSE(gizmos.IsGizmosDrawer(TypeOf(Component)));
}

TEST(SceneGizmos, TypesAreCollectedFromSceneWithoutDrawing)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    auto camera = mmake<CameraActor>();
    camera->SetPerspective(Math::Deg2rad(60.0f), 1.0f, 100.0f);
    TickScene();

    SceneGizmos gizmos;
    int typesChangedCalls = 0;
    gizmos.onGizmosTypesChanged = [&]() { typesChangedCalls++; };

    gizmos.UpdateGizmosTypes();

    EXPECT_TRUE(gizmos.GetGizmosTypes().Contains(&TypeOf(BoxCollider)));
    EXPECT_TRUE(gizmos.GetGizmosTypes().Contains(&TypeOf(CameraActor)));
    EXPECT_EQ(typesChangedCalls, 2);

    gizmos.UpdateGizmosTypes();
    EXPECT_EQ(typesChangedCalls, 2); // already known types are not reported again
}

TEST(SceneGizmos, TypesAreCollectedWhenDrawingDisabled)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    SceneGizmos gizmos;
    gizmos.SetEnabled(false);
    gizmos.SetTypeEnabled(&TypeOf(BoxCollider), false);

    EXPECT_TRUE(DrawAndCapture(gizmos).IsEmpty());
    EXPECT_TRUE(gizmos.GetGizmosTypes().Contains(&TypeOf(BoxCollider)));
}

TEST(SceneGizmos, DisabledTypeIsNotDrawn)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    SceneGizmos gizmos;
    ASSERT_FALSE(DrawAndCapture(gizmos).IsEmpty());

    gizmos.SetTypeEnabled(&TypeOf(BoxCollider), false);
    EXPECT_TRUE(DrawAndCapture(gizmos).IsEmpty());

    gizmos.SetTypeEnabled(&TypeOf(BoxCollider), true);
    EXPECT_FALSE(DrawAndCapture(gizmos).IsEmpty());
}

TEST(SceneGizmos, DisabledGizmosDrawNothing)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    SceneGizmos gizmos;
    gizmos.SetEnabled(false);

    EXPECT_TRUE(DrawAndCapture(gizmos).IsEmpty());
}

TEST(SceneGizmos, SelectionSwitchStaysLastBelowTypes)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    auto popup = mmake<GizmosPopup>();
    popup->Show(Vec2F(100, 100));

    auto children = popup->GetChildWidgets();
    ASSERT_GT(children.Count(), 2); // at least one gizmos type between the common switches

    EXPECT_EQ(children.First()->GetName(), "gizmos enable");
    EXPECT_EQ(children.Last()->GetName(), "selection enable");
    EXPECT_EQ(children[1]->GetName(), TypeOf(BoxCollider).GetName());

    // selection row sits below the last type row
    EXPECT_LT(children.Last()->layout->GetWorldRect().top, children[1]->layout->GetWorldRect().bottom + 1.0f);
}

TEST(SceneGizmos, SelectionOutlineCollectsComponentsWithChildren)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = mmake<Actor>();
    auto image = actor->AddComponent<ImageComponent>();

    auto child = mmake<Actor>();
    child->SetParent(actor);
    auto childImage = child->AddComponent<ImageComponent>();

    auto deepChild = mmake<Actor>();
    deepChild->SetParent(child);
    auto deepImage = deepChild->AddComponent<ImageComponent>();

    auto disabledChild = mmake<Actor>();
    disabledChild->SetParent(actor);
    auto disabledImage = disabledChild->AddComponent<ImageComponent>();
    disabledChild->SetEnabled(false);

    TickScene();

    ASSERT_EQ(image->GetSceneDrawableCategory(), SceneDrawableCategory::Scene2D);

    Vector<Ref<Component>> components;
    SceneEditScreen::CollectDrawableComponents(actor, SceneDrawableCategory::Scene2D, components);

    EXPECT_TRUE(components.Contains(DynamicCast<Component>(image)));
    EXPECT_TRUE(components.Contains(DynamicCast<Component>(childImage)));
    EXPECT_TRUE(components.Contains(DynamicCast<Component>(deepImage)));
    EXPECT_FALSE(components.Contains(DynamicCast<Component>(disabledImage)));
    EXPECT_EQ(components.Count(), 3);

    Vector<Ref<Component>> components3D;
    SceneEditScreen::CollectDrawableComponents(actor, SceneDrawableCategory::Scene3D, components3D);
    EXPECT_TRUE(components3D.IsEmpty());
}

TEST(SceneGizmos, SelectionOutlineCollectsEachComponentOnce)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = mmake<Actor>();
    auto child = mmake<Actor>();
    child->SetParent(actor);
    auto childImage = child->AddComponent<ImageComponent>();

    TickScene();

    // parent and its child selected together must not put the child content into the mask twice
    Vector<Ref<Component>> components;
    SceneEditScreen::CollectDrawableComponents(actor, SceneDrawableCategory::Scene2D, components);
    SceneEditScreen::CollectDrawableComponents(child, SceneDrawableCategory::Scene2D, components);

    EXPECT_EQ(components.Count(), 1);
}

TEST(SceneGizmos, PopupShowsToggleForEachDrawingType)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    MakeBoxColliderActor();

    auto& gizmos = o2EditorSceneScreen.GetGizmos();

    auto popup = mmake<GizmosPopup>();
    popup->Show(Vec2F(100, 100)); // collects scene types itself

    ASSERT_TRUE(gizmos.GetGizmosTypes().Contains(&TypeOf(BoxCollider)));

    auto toggles = DynamicCastVector<Toggle>(popup->GetChildWidgets());
    EXPECT_EQ(toggles.Count(), gizmos.GetGizmosTypes().Count() + 2); // common switches and one per type

    auto colliderToggle = toggles.FindOrDefault([](const Ref<Toggle>& x) {
        return x->GetName() == TypeOf(BoxCollider).GetName();
    });

    ASSERT_NE(colliderToggle, nullptr);
    EXPECT_TRUE(colliderToggle->GetValue());

    colliderToggle->onToggleByUser(false);
    EXPECT_FALSE(gizmos.IsTypeEnabled(&TypeOf(BoxCollider)));

    colliderToggle->onToggleByUser(true);
    EXPECT_TRUE(gizmos.IsTypeEnabled(&TypeOf(BoxCollider)));
}

TEST(SceneGizmos, ColliderGizmoIsDrawnOnScreen)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = mmake<Actor>();
    actor->transform->SetPosition2D(Vec2F(0, 0));
    actor->transform->SetSize2D(Vec2F(200, 100));
    actor->AddComponent<BoxCollider>();
    TickScene();

    SceneGizmos gizmos;

    Ref<Bitmap> captured;
    for (int frame = 0; frame < 3; frame++) // first frames have no presentable back buffer yet
    {
        if (frame == 2)
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.SetCamera(Camera::Default());
        o2Render.Clear(Color4(0, 0, 0, 255));
        gizmos.Draw([](const Vec3F& point) { return Vec2F(point.x, point.y); });
        o2Render.End();
    }

    ASSERT_NE(captured, nullptr);
    ASSERT_EQ(captured->GetFormat(), PixelFormat::R8G8B8A8);

    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();

    Vec2I min(size.x, size.y), max(-1, -1);
    int coloredPixels = 0;
    for (int y = 0; y < size.y; y++)
    {
        for (int x = 0; x < size.x; x++)
        {
            const UInt8* pixel = data + (y*size.x + x)*4;
            bool isGizmoColor = pixel[1] > 150 && pixel[0] < 150 && pixel[2] < 200 && pixel[3] > 100;
            if (!isGizmoColor)
                continue;

            coloredPixels++;
            min = Vec2I(Math::Min(min.x, x), Math::Min(min.y, y));
            max = Vec2I(Math::Max(max.x, x), Math::Max(max.y, y));
        }
    }

    ASSERT_GT(coloredPixels, 0);

    // outline is a 200x100 rectangle in the center of the screen
    EXPECT_NEAR((float)(max.x - min.x), 200.0f, 4.0f);
    EXPECT_NEAR((float)(max.y - min.y), 100.0f, 4.0f);
    EXPECT_NEAR((float)(max.x + min.x)*0.5f, (float)size.x*0.5f, 4.0f);
    EXPECT_NEAR((float)(max.y + min.y)*0.5f, (float)size.y*0.5f, 4.0f);
}

TEST(SceneGizmos, PopupCommonSwitchesControlGizmosAndSelection)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto popup = mmake<GizmosPopup>();
    popup->Show(Vec2F(100, 100));

    auto children = popup->GetChildWidgets();
    ASSERT_GE(children.Count(), 2);

    // common gizmos switch is first, selection switch is always last, under a separator line
    auto gizmosToggle = DynamicCast<Toggle>(children.First());
    auto selectionToggle = DynamicCast<Toggle>(children.Last());

    ASSERT_NE(gizmosToggle, nullptr);
    ASSERT_NE(selectionToggle, nullptr);
    EXPECT_EQ(gizmosToggle->GetName(), "gizmos enable");
    EXPECT_EQ(selectionToggle->GetName(), "selection enable");
    EXPECT_NE(selectionToggle->GetLayer("line"), nullptr);
    EXPECT_TRUE(gizmosToggle->GetValue());
    EXPECT_TRUE(selectionToggle->GetValue());

    gizmosToggle->onToggleByUser(false);
    EXPECT_FALSE(o2EditorSceneScreen.GetGizmos().IsEnabled());
    gizmosToggle->onToggleByUser(true);
    EXPECT_TRUE(o2EditorSceneScreen.GetGizmos().IsEnabled());

    selectionToggle->onToggleByUser(false);
    EXPECT_FALSE(o2EditorSceneScreen.IsSelectionVisible());
    selectionToggle->onToggleByUser(true);
    EXPECT_TRUE(o2EditorSceneScreen.IsSelectionVisible());
}
