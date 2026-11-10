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
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Windows/SceneWindow/GizmosPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneGizmos.h"
#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Vector<Ref<SceneEditableObject>> Selection(const Vector<Ref<Actor>>& actors)
    {
        return actors.Convert<Ref<SceneEditableObject>>([](const Ref<Actor>& x) {
            return DynamicCast<SceneEditableObject>(x);
        });
    }

    Vector<Vec3F> DrawAndCapture(SceneGizmos& gizmos, const Vector<Ref<Actor>>& selected)
    {
        Vector<Vec3F> points;
        gizmos.Draw(Selection(selected),
                    [&](const Vec3F& point)
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

TEST(SceneGizmos, TypesAreCollectedFromUnselectedObjectsToo)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = MakeBoxColliderActor();

    SceneGizmos gizmos;
    gizmos.UpdateGizmosTypes();

    // Only the selection draws, but the popup lists what the whole scene could draw
    EXPECT_TRUE(DrawAndCapture(gizmos, {}).IsEmpty());
    EXPECT_TRUE(gizmos.GetGizmosTypes().Contains(&TypeOf(BoxCollider)));
}

TEST(SceneGizmos, OnlySelectedObjectsAreDrawn)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto selected = MakeBoxColliderActor();
    selected->transform->SetPosition2D(Vec2F(100, 0));

    auto other = MakeBoxColliderActor();
    other->transform->SetPosition2D(Vec2F(-100, 0));
    TickScene();

    SceneGizmos gizmos;

    EXPECT_TRUE(DrawAndCapture(gizmos, {}).IsEmpty());

    auto points = DrawAndCapture(gizmos, { selected });
    ASSERT_FALSE(points.IsEmpty());

    for (auto& point : points)
        EXPECT_GT(point.x, 0.0f); // only the selected actor's outline, the other one sits at -100
}

TEST(SceneGizmos, SelectedObjectDrawsGizmosOfItsChildren)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto parent = mmake<Actor>();
    parent->transform->SetPosition2D(Vec2F(0, 0));

    auto child = MakeBoxColliderActor();
    child->SetParent(parent);
    child->transform->SetPosition2D(Vec2F(50, 0));
    TickScene();

    SceneGizmos gizmos;

    auto points = DrawAndCapture(gizmos, { parent });
    EXPECT_EQ(points.Count(), 4); // the child collider outline, drawn through the selected parent
}

TEST(SceneGizmos, SelectedParentAndChildDrawGizmosOnce)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto parent = MakeBoxColliderActor();
    auto child = MakeBoxColliderActor();
    child->SetParent(parent);
    TickScene();

    SceneGizmos gizmos;

    auto both = DrawAndCapture(gizmos, { parent, child });
    auto parentOnly = DrawAndCapture(gizmos, { parent });

    EXPECT_EQ(both.Count(), parentOnly.Count()); // the child is covered by its parent's subtree
}

TEST(SceneGizmos, DisabledTypeIsNotDrawn)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = MakeBoxColliderActor();

    SceneGizmos gizmos;
    ASSERT_FALSE(DrawAndCapture(gizmos, { actor }).IsEmpty());

    gizmos.SetTypeEnabled(&TypeOf(BoxCollider), false);
    EXPECT_TRUE(DrawAndCapture(gizmos, { actor }).IsEmpty());

    gizmos.SetTypeEnabled(&TypeOf(BoxCollider), true);
    EXPECT_FALSE(DrawAndCapture(gizmos, { actor }).IsEmpty());
}

TEST(SceneGizmos, DisabledGizmosDrawNothing)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = MakeBoxColliderActor();

    SceneGizmos gizmos;
    gizmos.SetEnabled(false);

    EXPECT_TRUE(DrawAndCapture(gizmos, { actor }).IsEmpty());
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

TEST(SceneGizmos, SelectionOutlineCollectsWidgetsWithChildren)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto panel = mmake<Widget>();
    auto image = mmake<Widget>();
    image->SetParent(panel);
    image->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto deepImage = mmake<Widget>();
    deepImage->SetParent(image);
    deepImage->AddLayer("icon", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto disabledImage = mmake<Widget>();
    disabledImage->SetParent(panel);
    disabledImage->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    disabledImage->SetEnabled(false);
    TickScene();

    // the panel draws nothing itself; a parent and its child selected together put the child in once
    Vector<Ref<Widget>> widgets;
    SceneEditScreen::CollectDrawableWidgets(panel, widgets);
    SceneEditScreen::CollectDrawableWidgets(image, widgets);

    EXPECT_TRUE(widgets.Contains(image));
    EXPECT_TRUE(widgets.Contains(deepImage));
    EXPECT_FALSE(widgets.Contains(panel));
    EXPECT_FALSE(widgets.Contains(disabledImage));
    EXPECT_EQ(widgets.Count(), 2);
}

// Draws the scene screen in a frame and counts the pixels of the selection outline colour
static int DrawSceneAndCountOutline()
{
    auto& screen = o2EditorSceneScreen;
    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    screen.NeedRedraw();
    screen.Draw();
    o2Render.End();

    if (!captured)
        return -1;

    Color4 outline = EditorTheme::Get().selectionOutline;
    Vec2I size = captured->GetSize();
    const UInt8* data = captured->GetData();
    int count = 0;
    for (int i = 0; i < size.x*size.y; i++)
    {
        const UInt8* pixel = data + i*4;
        if (Math::Abs(pixel[0] - outline.r) <= 12 && Math::Abs(pixel[1] - outline.g) <= 12 && Math::Abs(pixel[2] - outline.b) <= 12)
            count++;
    }
    return count;
}

// Game screens are built of widgets: the outline goes round the layers of a selected widget and of the
// widgets down its hierarchy, a parent without layers of its own included
TEST(SceneGizmos, SelectionOutlineGoesRoundWidgets)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;
    auto& screen = o2EditorSceneScreen;
    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();
    screen.SetView3DMode(false);
    screen.SetSelectionVisible(true);

    auto panel = mmake<Widget>();
    *panel->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(240.0f, 160.0f));
    auto image = mmake<Widget>();
    image->SetParent(panel);
    *image->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(120.0f, 80.0f));
    image->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    TickScene();

    auto sprite = mmake<Actor>();
    sprite->transform->SetPosition2D(Vec2F(-250.0f, 0.0f));
    sprite->transform->SetSize2D(Vec2F(80.0f, 80.0f));
    sprite->AddComponent<ImageComponent>();
    TickScene();

    screen.SelectObjectsByIdsWithoutAction({});
    int none = DrawSceneAndCountOutline();
    ASSERT_GE(none, 0);

    screen.SelectObjectsByIdsWithoutAction({ sprite->GetID() });
    EXPECT_GT(DrawSceneAndCountOutline(), none + 100) << "a sprite component is outlined";

    screen.SelectObjectsByIdsWithoutAction({ image->GetID() });
    int selectedImage = DrawSceneAndCountOutline();
    EXPECT_GT(selectedImage, none + 100) << "the outline goes round the selected widget's sprite";

    screen.SelectObjectsByIdsWithoutAction({ panel->GetID() });
    int selectedPanel = DrawSceneAndCountOutline();
    EXPECT_GT(selectedPanel, none + 100) << "and round the widgets in the selected hierarchy";

    image->GetLayer("back")->SetEnabled(false);
    TickScene();
    EXPECT_LE(DrawSceneAndCountOutline(), none + 4) << "a disabled layer isn't outlined";

    screen.SelectObjectsByIdsWithoutAction({});
}

// A widget layer selected by itself is outlined with its sublayers, the rest of the widget is not
TEST(SceneGizmos, SelectionOutlineGoesRoundSelectedLayers)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;
    auto& screen = o2EditorSceneScreen;
    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();
    screen.SetView3DMode(false);
    screen.SetSelectionVisible(true);

    auto widget = mmake<Widget>();
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(240.0f, 160.0f));
    widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto icon = widget->AddLayer("icon", mmake<Sprite>(Color4::White()), Layout::Based(BaseCorner::Center, Vec2F(60.0f, 60.0f)));
    auto badge = icon->AddChildLayer("badge", mmake<Sprite>(Color4::White()), Layout::Based(BaseCorner::Center, Vec2F(30.0f, 30.0f)));
    TickScene();

    screen.SelectObjectsByIdsWithoutAction({});
    int none = DrawSceneAndCountOutline();
    ASSERT_GE(none, 0);

    screen.SelectObjectsByIdsWithoutAction({ widget->GetID() });
    int wholeWidget = DrawSceneAndCountOutline();

    screen.SelectObjectsByIdsWithoutAction({ icon->GetID() });
    int iconLayer = DrawSceneAndCountOutline();
    EXPECT_GT(iconLayer, none + 100) << "the selected layer is outlined";
    EXPECT_LT(iconLayer, wholeWidget) << "by itself, not with the whole widget";

    screen.SelectObjectsByIdsWithoutAction({ badge->GetID() });
    int badgeLayer = DrawSceneAndCountOutline();
    EXPECT_GT(badgeLayer, none + 50) << "a sublayer is selected by itself too";
    EXPECT_LT(badgeLayer, iconLayer);

    screen.SelectObjectsByIdsWithoutAction({ icon->GetID() });
    icon->SetEnabled(false);
    EXPECT_LE(DrawSceneAndCountOutline(), none + 4) << "a disabled layer isn't outlined, nor its sublayers";

    screen.SelectObjectsByIdsWithoutAction({ badge->GetID() });
    EXPECT_LE(DrawSceneAndCountOutline(), none + 4) << "nor a sublayer selected by itself";

    screen.SelectObjectsByIdsWithoutAction({});
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
        gizmos.Draw(Selection({ actor }), [](const Vec3F& point) { return Vec2F(point.x, point.y); });
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

TEST(SceneGizmos, GeometryBehindCameraIsClippedInPerspectiveView)
{
    SceneCleanGuard guard;
    CreateModeGuard createModeGuard;

    auto actor = mmake<Actor>();
    actor->transform->SetPosition2D(Vec2F(0, -100));
    actor->transform->SetSize2D(Vec2F(200, 200));
    actor->AddComponent<BoxCollider>();
    TickScene();

    // Camera looks along the plane from inside the collider outline, so its far side is behind the camera
    SceneView3DState state;
    state.target = Vec3F();
    state.pitch = Math::Deg2rad(85.0f);
    state.distance = 100.0f;

    const Vec2F viewport(800.0f, 600.0f);

    Vec3F clipPlaneOrigin, clipPlaneNormal;
    state.GetNearClipPlane(clipPlaneOrigin, clipPlaneNormal);

    auto behindCamera = [&](const Vec3F& point) { return (point - clipPlaneOrigin).Dot(clipPlaneNormal) < 0.0f; };

    SceneGizmos gizmos;

    Vector<Vec3F> unclipped;
    gizmos.Draw(Selection({ actor }),
                [&](const Vec3F& point)
                {
                    unclipped.Add(point);
                    return state.WorldToScreen(point, viewport);
                });

    ASSERT_TRUE(unclipped.Contains(behindCamera)); // without clipping such points project mirrored

    Vector<Vec3F> clipped;
    gizmos.Draw(Selection({ actor }),
                [&](const Vec3F& point)
                {
                    clipped.Add(point);
                    return state.WorldToScreen(point, viewport);
                },
                clipPlaneOrigin, clipPlaneNormal);

    EXPECT_FALSE(clipped.IsEmpty());
    EXPECT_FALSE(clipped.Contains(behindCamera));
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
