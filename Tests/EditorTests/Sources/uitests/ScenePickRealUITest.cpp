#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/ScenePickMenu.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// The scene pick menu and the 3D orbit on the scenes of the project the tests run in, driven by the real mouse events and
// shot frame by frame. O2_PICK_REAL=<folder for the shots>; runs only with the project's built assets
namespace
{
    struct RealDriver
    {
        EditorShell& shell;
        String       dir;
        int          shot = 0;

        // One update and one draw a frame, as the editor's frame is: the listeners register once
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

        void Shoot(const String& name)
        {
            auto frame = Capture([&]() { shell.Draw(); });
            if (frame.bitmap)
                frame.bitmap->Save(dir + "/" + (String)(shot < 10 ? "0" : "") + (String)shot + "_" + name + ".png",
                                   Bitmap::ImageType::Png);
            shot++;
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
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(from, to, (float)i/(float)steps), 0);
                Step(1);
            }
            o2Input.OnAlt2CursorReleased();
            Step(2);
        }
    };

    String Describe(const Ref<SceneEditableObject>& object)
    {
        if (!object)
            return "null";

        return (String)object->GetName() + " <" + object->GetType().GetName() + ">";
    }

    void PrintMenu(const Ref<ScenePickMenu>& menu)
    {
        RectF rect = menu->layout->GetWorldRect();
        printf("menu: %d entries, rect %.0f %.0f %.0f %.0f, enabled %d\n", menu->GetEntries().Count(), rect.left,
               rect.bottom, rect.right, rect.top, (int)menu->IsEnabled());

        for (auto& entry : menu->GetEntries())
        {
            String hidden;
            for (auto& parent : entry.hiddenParents)
                hidden += (String)parent->GetName() + " / ";

            printf("  %*s%s   hidden: %s\n", entry.depth*2, "", Describe(entry.object).Data(), hidden.Data());
        }
    }

    Ref<Widget> MenuItemWidget(const Ref<ScenePickMenu>& menu, int index)
    {
        auto& items = menu->GetItems();
        if (index < 0 || index >= items.Count())
            return nullptr;

        return items[index]->widget.Lock();
    }
}

class ScenePickReal: public EditorVectorUIFixture
{
public:
    const char* outDir = nullptr;

    void SetUp() override
    {
        outDir = getenv("O2_PICK_REAL");
        if (!outDir)
            GTEST_SKIP() << "set O2_PICK_REAL to a folder for the shots";

        EditorVectorUIFixture::SetUp();
    }

    void TearDown() override
    {
        if (outDir)
            EditorVectorUIFixture::TearDown();
    }
};

// A screen of the game opened in the scene: a right click lists the widgets and layers under the cursor
TEST_F(ScenePickReal, GameScreenPrototype)
{
    const char* protoPath = getenv("O2_PICK_PROTO");
    String path = protoPath ? protoPath : "WordFall/UI/MainScreen.proto";
    if (!o2Assets.IsAssetExist(path))
        GTEST_SKIP() << path << " is not in the assets";

    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    RealDriver driver { shell, String(outDir) + "/proto" };

    Ref<Actor> screenActor;
    {
        ForcePopEditorScopeOnStack popScope;
        AssetRef<ActorAsset> proto(path);
        ASSERT_TRUE(proto);
        screenActor = proto->Instantiate();
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    driver.Step(5);

    // The whole screen in the view: the world rectangles of its widgets, their scale included
    auto worldRect = [](const Ref<Widget>& widget) {
        Basis basis = widget->layout->GetWorldBasis();
        Vec2F corners[] = { basis.origin, basis.origin + basis.xv, basis.origin + basis.yv, basis.origin + basis.xv + basis.yv };
        Vec2F minPoint = corners[0], maxPoint = corners[0];
        for (auto& corner : corners)
        {
            minPoint = Vec2F(Math::Min(minPoint.x, corner.x), Math::Min(minPoint.y, corner.y));
            maxPoint = Vec2F(Math::Max(maxPoint.x, corner.x), Math::Max(maxPoint.y, corner.y));
        }

        return RectF(minPoint, maxPoint);
    };

    RectF bounds;
    bool anyBounds = false;
    Function<void(const Ref<Actor>&)> collectBounds = [&](const Ref<Actor>& actor) {
        if (auto widget = DynamicCast<Widget>(actor))
        {
            if (widget->IsEnabledInHierarchy() && !widget->GetLayers().IsEmpty())
            {
                RectF rect = worldRect(widget);
                bounds = anyBounds ? bounds.Expand(rect) : rect;
                anyBounds = true;
            }
        }

        for (auto& child : actor->GetChildren())
            collectBounds(child);
    };
    collectBounds(screenActor);
    ASSERT_TRUE(anyBounds);
    Vec2F view = screen.layout->GetWorldRect().Size();
    float scale = Math::Max(bounds.Width()/view.x, bounds.Height()/view.y)*1.1f;
    Camera camera = screen.GetCamera();
    camera.SetPosition2D(bounds.Center());
    camera.SetScale2D(Vec2F(scale, scale));
    screen.SetCamera(camera);
    driver.Step(30);
    driver.Shoot("scene");

    // Every button of the screen under the cursor in turn
    Vector<Ref<Button>> buttons;
    Function<void(const Ref<Actor>&)> collect = [&](const Ref<Actor>& actor) {
        if (auto button = DynamicCast<Button>(actor))
            buttons.Add(button);

        for (auto& child : actor->GetChildren())
            collect(child);
    };
    collect(screenActor);
    printf("%d buttons in %s\n", buttons.Count(), path.Data());

    // The boosters first when the screen has them, else buttons across the screen
    Vector<Ref<Widget>> targets;
    Function<void(const Ref<Actor>&)> collectBoosters = [&](const Ref<Actor>& actor) {
        auto widget = DynamicCast<Widget>(actor);
        if (widget && widget->GetName().StartsWith("Booster") && widget->GetName() != "Boosters" &&
            widget->IsEnabledInHierarchy() && targets.Count() < 3)
        {
            targets.Add(widget);
        }

        for (auto& child : actor->GetChildren())
            collectBoosters(child);
    };
    collectBoosters(screenActor);

    for (int i = 0; i < buttons.Count() && targets.Count() < 3; i += Math::Max(1, buttons.Count()/3))
        targets.Add(buttons[i]);

    Vector<Vec2F> points;
    for (auto& button : targets)
        points.Add(screen.SceneToScreenPoint(worldRect(button).Center()));

    // Off the centre: the handle of the selection picked before stands there in the test editor, without the forwarding
    // to the scene the editor's handles have
    points.Add(screen.SceneToScreenPoint(bounds.Center()) + Vec2F(37.0f, -23.0f));

    for (auto& point : points)
    {
        auto under = screen.GetObjectsUnderPoint(point);
        printf("point %.0f %.0f: %d objects under it, drawn editable objects %d\n", point.x, point.y, under.Count(),
               o2Scene.GetDrawnEditableObjects().Count());
        for (auto& object : under)
            printf("   %s\n", Describe(object).Data());

        driver.RightClick(point);
        auto menu = screen.GetPickMenu();
        ASSERT_TRUE(menu);
        if (!menu->IsEnabled())
        {
            auto listeners = o2Events.GetAllCursorListenersUnderCursor(0);
            printf("no menu; under the cursor %d listeners:\n", listeners.Count());
            for (auto& listener : listeners)
            {
                auto widget = dynamic_cast<Widget*>(listener.Get());
                printf("   %s %s\n", typeid(*listener.Get()).name(), widget ? widget->GetName().Data() : "");
            }
        }
        PrintMenu(menu);
        driver.Shoot("menu");

        // The cursor goes down the items
        int count = Math::Min(menu->GetEntries().Count(), 6);
        for (int i = 0; i < count; i++)
        {
            auto item = MenuItemWidget(menu, menu->GetEntries().Count() - 1 - i);
            if (!item)
                continue;

            driver.Move(item->layout->GetWorldRect().Center());
            driver.Step(6);
            auto parentsMenu = menu->GetParentsMenu();
            RectF panel = parentsMenu && parentsMenu->IsEnabled() ? parentsMenu->layout->GetWorldRect() : RectF();
            printf("hover %d: %s, hovered %s, panel %.0f %.0f %.0f %.0f\n", menu->GetEntries().Count() - 1 - i,
                   Describe(menu->GetEntries()[menu->GetEntries().Count() - 1 - i].object).Data(),
                   Describe(screen.GetHoverObject()).Data(), panel.left, panel.bottom, panel.right, panel.top);
            driver.Shoot("hover");
        }

        // A click on the last item selects its object
        auto last = MenuItemWidget(menu, menu->GetEntries().Count() - 1);
        auto expected = menu->GetEntries().Last().object;
        driver.LeftClick(last->layout->GetWorldRect().Center());
        printf("picked: %s, selected %d: %s, menu enabled %d\n", Describe(expected).Data(),
               screen.GetSelectedObjects().Count(),
               screen.GetSelectedObjects().IsEmpty() ? "-" : Describe(screen.GetSelectedObjects()[0]).Data(),
               (int)menu->IsEnabled());
        EXPECT_FALSE(menu->IsEnabled());
        ASSERT_EQ(screen.GetSelectedObjects().Count(), 1);
        EXPECT_EQ(screen.GetSelectedObjects()[0], expected);
        driver.Step(5);
        driver.Shoot("picked");
    }

    ForcePopEditorScopeOnStack popScope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}

// A 3D scene: the pick menu in 3D and the orbit by the middle button around the selected object
TEST_F(ScenePickReal, Scene3D)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    RealDriver driver { shell, String(outDir) + "/scene3d" };

    auto makeMesh = [](const String& name, PrimitiveType3D type, const Vec3F& position, const Vec3F& size,
                       const Color4& color, const Ref<Actor>& parent) {
        ForcePopEditorScopeOnStack popScope;
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->name = name;
        if (parent)
            actor->SetParent(parent, false);

        auto mesh = actor->AddComponent<MeshPrimitiveComponent>();
        mesh->SetPrimitiveType(type);
        mesh->SetSize(size);
        mesh->SetColor(color);
        actor->transform->SetPosition(position);
        return actor;
    };

    Ref<Actor> level;
    {
        ForcePopEditorScopeOnStack popScope;
        level = mmake<Actor>(ActorCreateMode::InScene);
        level->name = "level";
    }
    makeMesh("ground", PrimitiveType3D::Plane, Vec3F(), Vec3F(600.0f, 600.0f, 1.0f), Color4(120, 160, 120), level);
    auto box = makeMesh("box", PrimitiveType3D::Box, Vec3F(0.0f, 0.0f, 50.0f), Vec3F(100.0f, 100.0f, 100.0f),
                        Color4(200, 120, 80), level);
    makeMesh("sphere", PrimitiveType3D::Sphere, Vec3F(180.0f, 60.0f, 40.0f), Vec3F(80.0f, 80.0f, 80.0f),
             Color4(80, 120, 200), level);
    {
        ForcePopEditorScopeOnStack popScope;
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(true);
    driver.Step(30);
    driver.Shoot("scene");

    auto& view = screen.GetView3DState();
    printf("view: target %.1f %.1f %.1f yaw %.2f pitch %.2f distance %.1f\n", view.target.x, view.target.y,
           view.target.z, view.yaw, view.pitch, view.distance);

    if (box)
    {
        Vec3F boxPosition = box->transform->GetWorldPosition();
        Vec2F boxPoint = screen.World3DToScreenPoint(boxPosition);
        printf("box at %.1f %.1f %.1f, screen %.0f %.0f\n", boxPosition.x, boxPosition.y, boxPosition.z, boxPoint.x,
               boxPoint.y);

        driver.RightClick(boxPoint);
        auto menu = screen.GetPickMenu();
        if (menu)
        {
            PrintMenu(menu);
            driver.Shoot("menu");

            for (int i = 0; i < menu->GetEntries().Count(); i++)
            {
                auto item = MenuItemWidget(menu, i);
                driver.Move(item->layout->GetWorldRect().Center());
                driver.Step(6);
                printf("hover %d: hovered %s\n", i, Describe(screen.GetHoverObject()).Data());
                driver.Shoot("hover");
            }

            driver.LeftClick(MenuItemWidget(menu, 0)->layout->GetWorldRect().Center());
            printf("selected %d\n", screen.GetSelectedObjects().Count());
        }
        else
            printf("no menu\n");

        // The box selected, the middle button turns the view around it
        screen.SelectObjects({ DynamicCast<SceneEditableObject>(box) }, false);
        driver.Step(5);
        driver.Shoot("selected");

        Vec2F start = screen.layout->GetWorldRect().Center();
        Vec2F before = screen.World3DToScreenPoint(boxPosition);
        driver.MiddleDrag(start, start + Vec2F(200.0f, 0.0f));
        Vec2F after = screen.World3DToScreenPoint(boxPosition);
        printf("orbit right: yaw %.2f pitch %.2f, box on screen %.0f %.0f -> %.0f %.0f\n", view.yaw, view.pitch,
               before.x, before.y, after.x, after.y);
        driver.Shoot("orbit_right");

        driver.MiddleDrag(start, start + Vec2F(0.0f, 150.0f));
        after = screen.World3DToScreenPoint(boxPosition);
        printf("orbit up: yaw %.2f pitch %.2f, box on screen %.0f %.0f\n", view.yaw, view.pitch, after.x, after.y);
        driver.Shoot("orbit_up");

        driver.MiddleDrag(start, start + Vec2F(-400.0f, -100.0f));
        after = screen.World3DToScreenPoint(boxPosition);
        printf("orbit left down: yaw %.2f pitch %.2f, box on screen %.0f %.0f\n", view.yaw, view.pitch, after.x,
               after.y);
        driver.Shoot("orbit_left_down");
    }

    screen.SetView3DMode(false);

    ForcePopEditorScopeOnStack popScope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}
