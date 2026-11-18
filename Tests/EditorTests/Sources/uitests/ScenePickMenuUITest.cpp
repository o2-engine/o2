#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/ScenePickMenu.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// The scene pick menu and the 3D orbit driven by the mouse events as the platform sends them, a frame drawn once
// between them as the editor draws it
namespace
{
    struct MouseDriver
    {
        EditorShell& shell;

        void Step(int frames = 1)
        {
            for (int i = 0; i < frames; i++)
            {
                shell.Update(1.0f/60.0f);
                {
                    PushEditorScopeOnStack scope;
                    o2Render.Begin();
                    o2Render.SetCamera(Camera());
                    shell.Draw();
                    o2UI.DrawCurrentLayerTopWidgets();
                    o2Render.End();
                }
                AppTestDriver::PumpFrames(1);
            }
        }

        void Move(const Vec2F& point, int steps = 6)
        {
            Vec2F from = o2Input.GetCursorPos();
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(from, point, (float)i/(float)steps), 0);
                Step(1);
            }
        }

        void RightClick(const Vec2F& point)
        {
            Move(point);
            o2Input.OnAltCursorPressed(point);
            Step(2);
            o2Input.OnAltCursorReleased();
            Step(4);
        }

        void RightDrag(const Vec2F& from, const Vec2F& to)
        {
            Move(from);
            o2Input.OnAltCursorPressed(from);
            Step(1);
            Move(to, 10);
            o2Input.OnAltCursorReleased();
            Step(4);
        }

        void LeftClick(const Vec2F& point)
        {
            Move(point);
            o2Input.OnCursorPressed(point);
            Step(2);
            o2Input.OnCursorReleased();
            Step(4);
        }

        void MiddleDrag(const Vec2F& from, const Vec2F& to, int steps = 20)
        {
            Move(from);
            o2Input.OnAlt2CursorPressed(from);
            Step(1);
            Move(to, steps);
            o2Input.OnAlt2CursorReleased();
            Step(4);
        }
    };

    Ref<Widget> MakeWidget(const String& name, const RectF& rect, const Ref<Widget>& parent)
    {
        ForcePopEditorScopeOnStack scope;

        auto widget = mmake<Widget>(ActorCreateMode::InScene);
        widget->name = name;
        if (parent)
            parent->AddChild(widget);

        *widget->layout = WidgetLayout::Based(BaseCorner::LeftBottom, rect.Size(), rect.LeftBottom());
        return widget;
    }

    Ref<WidgetLayer> AddLayer(const Ref<Widget>& widget, const String& name, float transparency = 1.0f)
    {
        ForcePopEditorScopeOnStack scope;

        auto layer = widget->AddLayer(name, mmake<Sprite>());
        layer->transparency = transparency;
        return layer;
    }

    // The center of a widget in the world: its layout rectangle carried by its own and its parents' scale
    Vec2F WorldCenter(const Ref<Widget>& widget)
    {
        Basis basis = widget->layout->GetWorldBasis();
        return basis.origin + (basis.xv + basis.yv)*0.5f;
    }

    Vec2F ItemCenter(const Ref<ScenePickMenu>& menu, int index)
    {
        auto widget = menu->GetItems()[index]->widget.Lock();
        return widget ? widget->layout->GetWorldRect().Center() : Vec2F();
    }

    int EntryOf(const Ref<ScenePickMenu>& menu, const Ref<SceneEditableObject>& object)
    {
        auto& entries = menu->GetEntries();
        for (int i = 0; i < entries.Count(); i++)
        {
            if (entries[i].object == object)
                return i;
        }

        return -1;
    }

    // A screen of widgets: a container without layers, a panel with a picture, a button with the layer of its pressed
    // state transparent, a label in a group without layers inside the button
    struct WidgetsScene
    {
        Ref<Widget> screen, panel, button, group, label;
        Ref<WidgetLayer> buttonBack, buttonPressed, labelText;

        // scale: of the screen, as a game screen fitted into the camera's frame by its root's scale
        explicit WidgetsScene(float scale = 1.0f)
        {
            screen = MakeWidget("screen", RectF(-300.0f, -400.0f, 300.0f, 400.0f), nullptr);
            if (scale != 1.0f)
            {
                ForcePopEditorScopeOnStack scope;
                screen->transform->SetScale2D(Vec2F(scale, scale));
            }

            panel = MakeWidget("panel", RectF(0.0f, 100.0f, 600.0f, 600.0f), screen);
            AddLayer(panel, "picture");
            button = MakeWidget("button", RectF(100.0f, 50.0f, 400.0f, 250.0f), panel);
            buttonBack = AddLayer(button, "back");
            buttonPressed = AddLayer(button, "pressed", 0.0f);
            group = MakeWidget("group", RectF(0.0f, 0.0f, 300.0f, 200.0f), button);
            label = MakeWidget("label", RectF(50.0f, 50.0f, 250.0f, 150.0f), group);
            labelText = AddLayer(label, "text");

            ForcePopEditorScopeOnStack scope;
            o2Scene.UpdateAddedEntities();
            o2Scene.UpdateTransforms();
        }

        ~WidgetsScene()
        {
            ForcePopEditorScopeOnStack scope;
            o2Scene.Clear(true);
            o2Scene.UpdateDestroyingEntities();
        }
    };
}

class ScenePickMenuUI: public EditorVectorUIFixture
{
public:
    bool layersVisible = false;

    void SetUp() override
    {
        EditorVectorUIFixture::SetUp();
        layersVisible = Widget::isEditorLayersVisible;
        Widget::isEditorLayersVisible = true;
    }

    void TearDown() override
    {
        Widget::isEditorLayersVisible = layersVisible;
        EditorVectorUIFixture::TearDown();
    }
};

// A right click in a moved and zoomed 2D view opens the menu at the cursor with what is seen there: widgets with their
// visible layers; a widget without layers and the transparent layer of another state are left out
TEST_F(ScenePickMenuUI, RightClickListsWhatIsSeenUnderTheCursor)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };
    WidgetsScene scene;

    // An emitter of an idle pool and a camera whose frame covers everything: neither draws at the cursor
    Ref<Actor> pool;
    Ref<CameraActor> sceneCamera;
    {
        ForcePopEditorScopeOnStack scope;
        pool = mmake<Actor>(ActorCreateMode::InScene);
        pool->name = "pool";
        pool->AddComponent(mmake<ParticlesEmitterComponent>());
        pool->transform->SetSize2D(Vec2F(2000.0f, 2000.0f));

        sceneCamera = mmake<CameraActor>();
        sceneCamera->name = "ui camera";
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    Camera camera = screen.GetCamera();
    camera.SetPosition2D(Vec2F(60.0f, 40.0f));
    camera.SetScale2D(Vec2F(1.7f, 1.7f));
    screen.SetCamera(camera);
    mouse.Step(10);

    Vec2F point = screen.SceneToScreenPoint(scene.label->layout->GetWorldRect().Center());
    mouse.RightClick(point);

    auto menu = screen.GetPickMenu();
    ASSERT_TRUE(menu);
    ASSERT_TRUE(menu->IsEnabled()) << "the click's point is in the screen space, the view's events come in the scene's";
    EXPECT_NEAR(menu->layout->GetWorldRect().left, point.x, 1.0f);
    EXPECT_NEAR(menu->layout->GetWorldRect().top, point.y, 1.0f);

    Vector<Ref<SceneEditableObject>> objects;
    for (auto& entry : menu->GetEntries())
        objects.Add(entry.object);

    auto as = [](auto x) { return DynamicCast<SceneEditableObject>(x); };
    EXPECT_TRUE(objects.Contains(as(scene.panel)));
    EXPECT_TRUE(objects.Contains(as(scene.button)));
    EXPECT_TRUE(objects.Contains(as(scene.buttonBack)));
    EXPECT_TRUE(objects.Contains(as(scene.label)));
    EXPECT_TRUE(objects.Contains(as(scene.labelText)));
    EXPECT_FALSE(objects.Contains(as(scene.screen))) << "a widget without layers is a container, not under the cursor";
    EXPECT_FALSE(objects.Contains(as(scene.group)));
    EXPECT_FALSE(objects.Contains(as(scene.buttonPressed))) << "the pressed state's layer is transparent";
    EXPECT_FALSE(objects.Contains(as(pool))) << "an emitter without live particles draws nothing";
    EXPECT_FALSE(objects.Contains(as(sceneCamera)));

    // The tree: the button inside the panel, the label inside the button
    auto& entries = menu->GetEntries();
    EXPECT_EQ(entries[EntryOf(menu, as(scene.panel))].depth, 0);
    EXPECT_EQ(entries[EntryOf(menu, as(scene.button))].depth, 1);
    EXPECT_EQ(entries[EntryOf(menu, as(scene.label))].depth, 2);

    menu->Hide(true);
}

// A screen scaled by its root, as the game's screens are in the camera's frame: its widgets and layers are where they
// are drawn, not where their unscaled layout rectangles are
TEST_F(ScenePickMenuUI, ScaledScreenListsItsWidgetsAndLayers)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };
    WidgetsScene scene(1179.0f/868.0f);

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    Camera camera = screen.GetCamera();
    camera.SetPosition2D(Vec2F(0.0f, 0.0f));
    camera.SetScale2D(Vec2F(2.0f, 2.0f));
    screen.SetCamera(camera);
    mouse.Step(10);

    // A layer's transform is the world rectangle its drawable is drawn in
    Basis layerBasis = scene.labelText->GetTransform();
    Basis drawnBasis = scene.labelText->GetDrawable()->GetBasis();
    EXPECT_NEAR(layerBasis.origin.x, drawnBasis.origin.x, 0.5f);
    EXPECT_NEAR(layerBasis.origin.y, drawnBasis.origin.y, 0.5f);
    EXPECT_NEAR(layerBasis.xv.Length(), drawnBasis.xv.Length(), 0.5f);
    EXPECT_NEAR(layerBasis.yv.Length(), drawnBasis.yv.Length(), 0.5f);

    // The label scaled away from its layout rectangle: the cursor is on it only in the world
    Vec2F labelCenter = WorldCenter(scene.label);
    ASSERT_FALSE(scene.label->layout->GetWorldRect().IsInside(labelCenter)) << "the scale moves the label far enough";
    mouse.RightClick(screen.SceneToScreenPoint(labelCenter));

    auto menu = screen.GetPickMenu();
    ASSERT_TRUE(menu && menu->IsEnabled());

    auto as = [](auto x) { return DynamicCast<SceneEditableObject>(x); };
    Vector<Ref<SceneEditableObject>> objects;
    for (auto& entry : menu->GetEntries())
        objects.Add(entry.object);

    EXPECT_TRUE(objects.Contains(as(scene.panel)));
    EXPECT_TRUE(objects.Contains(as(scene.button)));
    EXPECT_TRUE(objects.Contains(as(scene.buttonBack)));
    EXPECT_TRUE(objects.Contains(as(scene.label)));
    EXPECT_TRUE(objects.Contains(as(scene.labelText)));
    EXPECT_FALSE(objects.Contains(as(scene.buttonPressed)));

    menu->Hide(true);
}

// The cursor over an item outlines its object; over an item with skipped ancestors the parents menu opens on the left
// of the menu, its items outline and pick the ancestors; a click picks, the menus close and the outline goes
TEST_F(ScenePickMenuUI, HoverOutlinesAndTheParentsMenuPicks)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };
    WidgetsScene scene;

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    mouse.Step(10);

    auto as = [](auto x) { return DynamicCast<SceneEditableObject>(x); };
    mouse.RightClick(screen.SceneToScreenPoint(scene.label->layout->GetWorldRect().Center()));
    auto menu = screen.GetPickMenu();
    ASSERT_TRUE(menu && menu->IsEnabled());
    auto parents = menu->GetParentsMenu();
    ASSERT_TRUE(parents);

    // The panel: the screen above it is not under the cursor
    int panelIndex = EntryOf(menu, as(scene.panel));
    ASSERT_GE(panelIndex, 0);
    mouse.Move(ItemCenter(menu, panelIndex));
    mouse.Step(3);
    EXPECT_EQ(screen.GetHoverObject(), as(scene.panel));
    ASSERT_TRUE(parents->IsEnabled());
    ASSERT_EQ(parents->GetEntries().Count(), 1);
    EXPECT_EQ(parents->GetEntries()[0].object, as(scene.screen));
    EXPECT_LT(parents->layout->GetWorldRect().right, menu->layout->GetWorldRect().left + 0.5f);

    // The button: nothing skipped above it but the screen
    int buttonIndex = EntryOf(menu, as(scene.button));
    mouse.Move(ItemCenter(menu, buttonIndex));
    mouse.Step(3);
    EXPECT_EQ(screen.GetHoverObject(), as(scene.button));

    // The label: the screen and the group are skipped, the group indented under the screen
    int labelIndex = EntryOf(menu, as(scene.label));
    mouse.Move(ItemCenter(menu, labelIndex));
    mouse.Step(3);
    EXPECT_EQ(screen.GetHoverObject(), as(scene.label));
    ASSERT_TRUE(parents->IsEnabled());
    ASSERT_EQ(parents->GetEntries().Count(), 2);
    EXPECT_EQ(parents->GetEntries()[0].object, as(scene.screen));
    EXPECT_EQ(parents->GetEntries()[1].object, as(scene.group));
    EXPECT_GT(parents->GetEntries()[1].depth, parents->GetEntries()[0].depth);
    // The ancestors go up from the hovered item: the root on top, the last one ends where the item begins
    auto lastParent = parents->GetItems().Last()->widget.Lock();
    auto labelItem = menu->GetItems()[labelIndex]->widget.Lock();
    EXPECT_NEAR(lastParent->layout->GetWorldRect().bottom, labelItem->layout->GetWorldRect().top, 1.0f);
    EXPECT_GT(ItemCenter(parents, 0).y, ItemCenter(parents, 1).y);

    // Over the parents menu: its item is outlined, the label stays hovered in the menu
    mouse.Move(Vec2F(menu->layout->GetWorldRect().left - 10.0f, ItemCenter(menu, labelIndex).y), 4);
    mouse.Move(ItemCenter(parents, 1));
    mouse.Step(3);
    EXPECT_EQ(menu->GetHoveredEntryIndex(), labelIndex);
    EXPECT_TRUE(parents->IsEnabled());
    EXPECT_EQ(screen.GetHoverObject(), as(scene.group));

    // A click there picks the group
    mouse.LeftClick(ItemCenter(parents, 1));
    EXPECT_FALSE(menu->IsEnabled());
    EXPECT_FALSE(parents->IsEnabled());
    ASSERT_EQ(screen.GetSelectedObjects().Count(), 1);
    EXPECT_EQ(screen.GetSelectedObjects()[0], as(scene.group));
    EXPECT_FALSE(screen.GetHoverObject());

    // A click on an item of the menu while the parents menu is open picks that item
    mouse.RightClick(screen.SceneToScreenPoint(scene.label->layout->GetWorldRect().Center()));
    ASSERT_TRUE(menu->IsEnabled());
    labelIndex = EntryOf(menu, as(scene.label));
    mouse.Move(ItemCenter(menu, labelIndex));
    mouse.Step(3);
    ASSERT_TRUE(parents->IsEnabled());
    mouse.LeftClick(ItemCenter(menu, labelIndex));
    EXPECT_FALSE(menu->IsEnabled());
    EXPECT_FALSE(parents->IsEnabled());
    ASSERT_EQ(screen.GetSelectedObjects().Count(), 1);
    EXPECT_EQ(screen.GetSelectedObjects()[0], as(scene.label));

    // A click outside closes both and selects nothing
    mouse.RightClick(screen.SceneToScreenPoint(scene.label->layout->GetWorldRect().Center()));
    mouse.Move(ItemCenter(menu, EntryOf(menu, as(scene.label))));
    mouse.Step(3);
    ASSERT_TRUE(parents->IsEnabled());
    Vec2F outside = screen.layout->GetWorldRect().RightTop() - Vec2F(10.0f, 10.0f);
    mouse.LeftClick(outside);
    EXPECT_FALSE(menu->IsEnabled());
    EXPECT_FALSE(parents->IsEnabled());
    EXPECT_FALSE(screen.GetHoverObject());
}

// A right drag pans the 2D view and opens no menu
TEST_F(ScenePickMenuUI, RightDragPansWithoutTheMenu)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };
    WidgetsScene scene;

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    mouse.Step(10);

    Vec2F point = screen.SceneToScreenPoint(scene.label->layout->GetWorldRect().Center());
    Vec2F before = screen.GetCamera().GetPosition2D();
    mouse.RightDrag(point, point + Vec2F(120.0f, 40.0f));
    mouse.Step(30);

    EXPECT_GT((screen.GetCamera().GetPosition2D() - before).Length(), 20.0f);
    EXPECT_FALSE(screen.GetPickMenu() && screen.GetPickMenu()->IsEnabled());
}

// In 3D the middle button turns the view around the selected object, which keeps its place on the screen: right
// turns the yaw, up the pitch; letting it go opens no menu
TEST_F(ScenePickMenuUI, MiddleButtonOrbitsAroundTheSelection)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };

    Ref<Actor> box;
    {
        ForcePopEditorScopeOnStack scope;
        box = mmake<Actor>(ActorCreateMode::InScene);
        box->name = "box";
        auto mesh = box->AddComponent<MeshPrimitiveComponent>();
        mesh->SetPrimitiveType(PrimitiveType3D::Box);
        mesh->SetSize(Vec3F(100.0f, 100.0f, 100.0f));
        box->transform->SetPosition(Vec3F(120.0f, 40.0f, 50.0f));
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(true);
    auto& view = screen.GetView3DState();
    view.target = Vec3F();
    view.yaw = 0.0f;
    view.pitch = Math::Deg2rad(40.0f);
    view.distance = 600.0f;
    mouse.Step(5);

    // A right click there first: a later middle button must not open the menu
    Vec2F boxPoint = screen.World3DToScreenPoint(box->transform->GetWorldPosition());
    mouse.RightClick(boxPoint);
    ASSERT_TRUE(screen.GetPickMenu() && screen.GetPickMenu()->IsEnabled());
    screen.GetPickMenu()->Hide(true);

    screen.SelectObjects({ DynamicCast<SceneEditableObject>(box) }, false);
    mouse.Step(3);

    // Over the box all along: letting the button go over an object is where a right click would open the menu
    Vec2F pivotBefore = screen.World3DToScreenPoint(box->transform->GetWorldPosition());
    float yaw = view.yaw, pitch = view.pitch;

    mouse.MiddleDrag(pivotBefore - Vec2F(40.0f, 0.0f), pivotBefore + Vec2F(40.0f, 0.0f));
    EXPECT_LT(view.yaw, yaw - 0.2f) << "dragging right turns the view to the right";
    EXPECT_NEAR(view.pitch, pitch, 0.001f);
    Vec2F pivotAfter = screen.World3DToScreenPoint(box->transform->GetWorldPosition());
    EXPECT_NEAR(pivotAfter.x, pivotBefore.x, 1.0f);
    EXPECT_NEAR(pivotAfter.y, pivotBefore.y, 1.0f);

    mouse.MiddleDrag(pivotBefore - Vec2F(0.0f, 40.0f), pivotBefore + Vec2F(0.0f, 40.0f));
    EXPECT_GT(view.pitch, pitch + 0.2f) << "dragging up looks from lower";
    pivotAfter = screen.World3DToScreenPoint(box->transform->GetWorldPosition());
    EXPECT_NEAR(pivotAfter.x, pivotBefore.x, 1.0f);
    EXPECT_NEAR(pivotAfter.y, pivotBefore.y, 1.0f);

    EXPECT_FALSE(screen.GetPickMenu()->IsEnabled()) << "letting the middle button go is not a right click";

    screen.SetView3DMode(false);
    ForcePopEditorScopeOnStack scope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}

// An object picked deep in a big hierarchy: the tree expands its ancestors, scrolls to its node, selects and highlights it
TEST_F(ScenePickMenuUI, PickedDeepObjectIsShownInTheTree)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    MouseDriver mouse { shell };

    // Every level of the branch among many siblings, the leaf the only one with a layer
    const int levels = 8, siblings = 40;
    Ref<Widget> root = MakeWidget("root", RectF(-300.0f, -300.0f, 300.0f, 300.0f), nullptr);
    Ref<Widget> parent = root;
    for (int level = 0; level < levels; level++)
    {
        Ref<Widget> next;
        for (int i = 0; i < siblings; i++)
        {
            auto child = MakeWidget("level " + (String)level + " item " + (String)i, RectF(0.0f, 0.0f, 600.0f, 600.0f), parent);
            if (i == siblings/2)
                next = child;
        }
        parent = next;
    }
    Ref<Widget> leaf = parent;
    AddLayer(leaf, "picture");
    {
        ForcePopEditorScopeOnStack scope;
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    auto tree = o2EditorTree.GetSceneTree();
    tree->UpdateNodesView();
    mouse.Step(10);

    auto as = [](auto x) { return DynamicCast<SceneEditableObject>(x); };
    mouse.RightClick(screen.SceneToScreenPoint(leaf->layout->GetWorldRect().Center()));
    auto menu = screen.GetPickMenu();
    ASSERT_TRUE(menu && menu->IsEnabled());
    int leafIndex = EntryOf(menu, as(leaf));
    ASSERT_GE(leafIndex, 0);
    mouse.LeftClick(ItemCenter(menu, leafIndex));
    mouse.Step(10);

    ASSERT_EQ(screen.GetSelectedObjects().Count(), 1);
    EXPECT_EQ(screen.GetSelectedObjects()[0], as(leaf));

    auto node = tree->GetNode(as(leaf));
    ASSERT_TRUE(node) << "the leaf's node is in the tree's view";
    RectF nodeRect = node->layout->GetWorldRect();
    RectF treeRect = tree->layout->GetWorldRect();
    EXPECT_GE(nodeRect.bottom, treeRect.bottom - 0.5f);
    EXPECT_LE(nodeRect.top, treeRect.top + 0.5f);
    EXPECT_TRUE(node->IsSelected());
    EXPECT_TRUE(tree->GetSelectedObjects().Contains(as(leaf)));

    // The highlight shows after a pause of the editor's style
    mouse.Step(30);
    nodeRect = node->layout->GetWorldRect();
    auto highlight = tree->GetHighlightRectDrawable();
    ASSERT_TRUE(highlight);
    EXPECT_GT(highlight->GetTransparency(), 0.5f) << "the highlight is playing";
    EXPECT_NEAR(highlight->GetRect().Center().y, nodeRect.Center().y, 2.0f) << "the highlight is on the leaf's node";

    // The leaf's layer: its node is under the collapsed node of the leaf
    auto layer = leaf->GetLayer("picture");
    mouse.RightClick(screen.SceneToScreenPoint(leaf->layout->GetWorldRect().Center()));
    ASSERT_TRUE(menu->IsEnabled());
    int layerIndex = EntryOf(menu, as(layer));
    ASSERT_GE(layerIndex, 0);
    mouse.LeftClick(ItemCenter(menu, layerIndex));
    mouse.Step(10);

    auto layerNode = tree->GetNode(as(layer));
    ASSERT_TRUE(layerNode);
    RectF layerRect = layerNode->layout->GetWorldRect();
    EXPECT_GE(layerRect.bottom, treeRect.bottom - 0.5f);
    EXPECT_LE(layerRect.top, treeRect.top + 0.5f);
    EXPECT_TRUE(layerNode->IsSelected());

    ForcePopEditorScopeOnStack scope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}
