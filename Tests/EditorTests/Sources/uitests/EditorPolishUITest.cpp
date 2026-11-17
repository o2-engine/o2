#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Physics/BoxCollider.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Properties/Objects/Assets/FontStyleAssetViewer.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/UI/Preview/AssetPreviewIcon.h"
#include "o2Editor/UI/Preview/FontSampleView.h"
#include "o2Editor/UI/Preview/PrototypePreview.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/SceneWindow/GizmosPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/ScenePickMenu.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/SceneWindow/ViewModeSwitch.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/UI/SpoilerWithHead.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    template<typename _widget_type>
    Vector<Ref<_widget_type>> FindAllByType(const Ref<Widget>& widget)
    {
        Vector<Ref<_widget_type>> res;
        if (auto typed = DynamicCast<_widget_type>(widget))
            res.Add(typed);

        for (auto& child : widget->GetChildWidgets())
            res.Add(FindAllByType<_widget_type>(child));

        return res;
    }

    // Draws the widget alone as a frame does
    void DrawAlone(const Ref<Widget>& widget)
    {
        widget->Update(0.016f);
        widget->UpdateChildren(0.016f);
        widget->UpdateSelfTransform();
        widget->UpdateChildrenTransforms();
        o2Render.Begin();
        o2Render.Clear(Color4(20, 20, 20, 255));
        o2Render.SetCamera(Camera());
        widget->Draw();
        o2Render.End();
    }

    template<typename _viewer>
    Ref<_viewer> BuildViewer(IObject* object, Ref<VerticalLayout>& parent)
    {
        auto viewer = mmake<_viewer>();
        parent = o2UI.CreateWidget<VerticalLayout>();
        *parent->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(400.0f, 420.0f), Vec2F());
        viewer->CheckCreateSpoiler(parent);
        viewer->SetHeaderEnabled(false);
        viewer->Refresh({ { object, nullptr } });
        parent->SetEnabledForcible(true);
        parent->UpdateSelfTransform();
        parent->UpdateChildrenTransforms();
        return viewer;
    }

    // An image actor in the scene, laid at the position with the size
    Ref<Actor> ImageActor(const String& name, const Vec2F& position, const Vec2F& size, const Ref<Actor>& parent = nullptr)
    {
        ForcePopEditorScopeOnStack scope;

        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->name = name;
        actor->AddComponent(mmake<ImageComponent>(AssetRef<ImageAsset>("gem.png")));
        if (parent)
            actor->SetParent(parent, false);

        actor->transform->SetPosition2D(position);
        actor->transform->SetSize2D(size);
        return actor;
    }
}

class EditorPolish: public EditorVectorUIFixture
{};

// A 9-slice image is previewed whole in an asset field: its slices are for stretching, not for a small preview
TEST_F(EditorPolish, AssetFieldShowsANineSliceImageWhole)
{
    BuildStyle(false);
    PushEditorScopeOnStack scope;

    AssetRef<ImageAsset> image("gem.png");
    ASSERT_TRUE(image);

    auto meta = image->GetMeta();
    auto previousMode = meta->defaultMode;
    auto previousBorder = meta->sliceBorder;
    meta->defaultMode = SpriteMode::Sliced;
    meta->sliceBorder = BorderI(40, 40, 40, 40);

    auto icon = mmake<AssetPreviewIcon>();
    icon->SetAsset(AssetRef<Asset>(image));

    auto sprite = DynamicCast<Sprite>(icon->GetPreviewDrawable());
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->GetMode(), SpriteMode::Default);

    meta->defaultMode = previousMode;
    meta->sliceBorder = previousBorder;
}

// A prototype is rendered into a thumbnail once; the next asks are served from the cache
TEST_F(EditorPolish, PrototypeThumbnailIsRenderedOnceAndCached)
{
    BuildStyle(false);
    PushEditorScopeOnStack scope;
    PrototypePreview::ClearCache();

    auto widget = mmake<Widget>();
    widget->name = "card";
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200.0f, 100.0f));
    widget->AddLayer("back", mmake<Sprite>(AssetRef<ImageAsset>("gem.png")));
    AssetRef<ActorAsset> prototype(mmake<ActorAsset>(widget));

    EXPECT_TRUE(PrototypePreview::IsThumbnailPending(prototype));

    int renderedBefore = PrototypePreview::GetRenderedCount();
    TextureRef thumbnail;
    o2Render.Begin();
    thumbnail = PrototypePreview::GetThumbnail(prototype);
    auto again = PrototypePreview::GetThumbnail(prototype);
    o2Render.End();

    ASSERT_TRUE(thumbnail);
    EXPECT_EQ(again, thumbnail);
    EXPECT_EQ(PrototypePreview::GetRenderedCount(), renderedBefore + 1);
    EXPECT_FALSE(PrototypePreview::IsThumbnailPending(prototype));
    EXPECT_EQ(PrototypePreview::FindThumbnail(prototype), thumbnail);

    Vec2I size = thumbnail->GetSize();
    EXPECT_EQ(Math::Max(size.x, size.y), PrototypePreview::thumbnailSize);
    EXPECT_NEAR((float)size.x/(float)size.y, 2.0f, 0.05f);

    // The prototype itself is not laid out or changed by the thumbnail
    EXPECT_FALSE(widget->GetParent());

    // The asset field shows the thumbnail on the checked background
    auto icon = mmake<AssetPreviewIcon>();
    icon->SetAsset(AssetRef<Asset>(prototype));
    EXPECT_EQ(icon->GetKind(), AssetPreviewIcon::Kind::Prototype);
    *icon->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(40.0f, 40.0f));
    icon->SetEnabledForcible(true);
    DrawAlone(icon);

    auto sprite = DynamicCast<Sprite>(icon->GetPreviewDrawable());
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->GetTexture(), thumbnail);
    EXPECT_EQ(PrototypePreview::GetRenderedCount(), renderedBefore + 1);
}

// A prototype without anything to draw keeps the icon of its type
TEST_F(EditorPolish, PrototypeWithoutContentHasNoThumbnail)
{
    BuildStyle(false);
    PushEditorScopeOnStack scope;

    AssetRef<ActorAsset> prototype(mmake<ActorAsset>(mmake<Actor>()));

    o2Render.Begin();
    auto thumbnail = PrototypePreview::GetThumbnail(prototype);
    o2Render.End();

    EXPECT_FALSE(thumbnail);
    EXPECT_FALSE(PrototypePreview::IsThumbnailPending(prototype));
}

// A text style is previewed by lines of text drawn with it, in the Properties and in an asset field
TEST_F(EditorPolish, FontStyleIsPreviewedWithItsLines)
{
    BuildStyle(false);
    PushEditorScopeOnStack scope;

    auto style = mmake<FontStyleAsset>();
    style->AddEffect<FontColorEffect>(Color4(250, 200, 40, 255));
    style->AddEffect<FontStrokeEffect>(2.0f, Color4(120, 60, 0, 255));

    Ref<VerticalLayout> parent;
    auto viewer = BuildViewer<FontStyleAssetViewer>(style.Get(), parent);

    auto sample = parent->FindChildByTypeAndName<FontSampleView>("font style sample");
    ASSERT_TRUE(sample);
    ASSERT_FALSE(sample->GetLines().IsEmpty());
    EXPECT_TRUE(sample->GetFont());
    for (auto& line : sample->GetLines())
    {
        EXPECT_EQ(line->GetFontStyle(), Ref<FontStyle>(style));
        EXPECT_EQ(line->GetColor(), Color4::White()) << "a style that paints the glyphs keeps its colours";
    }

    auto icon = mmake<AssetPreviewIcon>();
    icon->SetAsset(AssetRef<Asset>(AssetRef<FontStyleAsset>(style)));
    EXPECT_EQ(icon->GetKind(), AssetPreviewIcon::Kind::FontStyle);
    auto text = DynamicCast<Text>(icon->GetPreviewDrawable());
    ASSERT_TRUE(text);
    EXPECT_EQ(text->GetFontStyle(), Ref<FontStyle>(style));
}

// The play badge of a video icon does not stay on the icons the grid reuses for other assets
TEST_F(EditorPolish, PlayBadgeStaysOnTheVideosOnly)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto& assets = o2EditorAssets;
    auto scroll = assets.GetAssetsScroll();
    assets.SetViewMode(0);
    assets.OpenFolder("Generated/GeminiShowcase");
    shell.Settle();

    int videos = 0;
    for (auto& icon : scroll->GetVisibleIcons())
        videos += icon->GetAssetInfo()->path.EndsWith(".mp4") ? 1 : 0;

    ASSERT_GT(videos, 0);

    for (const String& folder : { String(""), String("Generated/GeminiShowcase"), String("Generated") })
    {
        assets.OpenFolder(folder);
        shell.Settle();

        for (auto& icon : scroll->GetVisibleIcons())
        {
            bool isVideo = icon->GetAssetInfo()->path.EndsWith(".mp4");
            auto badge = icon->FindLayer("mediaBadge");
            if (!badge)
            {
                EXPECT_FALSE(isVideo) << icon->GetAssetInfo()->path;
                continue;
            }

            for (auto& layer : { badge, badge->FindChild("fill"), badge->FindChild("fill")->FindChild("glyph") })
                EXPECT_EQ(layer->IsEnabledWithParentLayers(), isVideo) << icon->GetAssetInfo()->path << " " << layer->name;
        }
    }
}

// A right click in the scene lists the objects under the cursor; the hovered one is outlined, a click selects it
TEST_F(EditorPolish, ScenePickMenuHoversAndSelects)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto parent = ImageActor("panel", Vec2F(), Vec2F(200.0f, 200.0f));
    auto between = mmake<Actor>(ActorCreateMode::InScene);
    {
        ForcePopEditorScopeOnStack popScope;
        between->name = "group";
        between->SetParent(parent, false);
    }
    auto child = ImageActor("icon", Vec2F(), Vec2F(50.0f, 50.0f), between);
    auto aside = ImageActor("aside", Vec2F(500.0f, 0.0f), Vec2F(50.0f, 50.0f));
    o2Scene.UpdateAddedEntities();
    o2Scene.UpdateTransforms();

    auto& screen = o2EditorSceneScreen;
    screen.NeedRedraw();
    shell.Settle();

    Vec2F point = screen.SceneToScreenPoint(child->transform->GetWorldPosition2D());
    auto objects = screen.GetObjectsUnderPoint(point);
    EXPECT_TRUE(objects.Contains(DynamicCast<SceneEditableObject>(parent)));
    EXPECT_TRUE(objects.Contains(DynamicCast<SceneEditableObject>(child)));
    EXPECT_FALSE(objects.Contains(DynamicCast<SceneEditableObject>(aside)));
    EXPECT_FALSE(objects.Contains(DynamicCast<SceneEditableObject>(between))) << "a parent not under the cursor is left out";

    ASSERT_TRUE(screen.ShowPickMenu(point));
    auto menu = screen.GetPickMenu();
    ASSERT_TRUE(menu && menu->IsEnabled());

    auto& entries = menu->GetEntries();
    ASSERT_EQ(entries.Count(), 2);
    EXPECT_EQ(entries[0].object, DynamicCast<SceneEditableObject>(parent));
    EXPECT_EQ(entries[1].object, DynamicCast<SceneEditableObject>(child));
    EXPECT_EQ(entries[1].depth, 1);

    menu->SetHoveredEntry(1);
    shell.Settle();
    EXPECT_EQ(screen.GetHoverObject(), DynamicCast<SceneEditableObject>(child));
    EXPECT_GT(menu->GetParentsPanelRect().Width(), 0.0f) << "the group between the panel and the icon is listed aside";
    EXPECT_LT(menu->GetParentsPanelRect().right, menu->layout->GetWorldRect().left + 1.0f);

    menu->PickEntry(1);
    shell.Settle();
    ASSERT_EQ(screen.GetSelectedObjects().Count(), 1);
    EXPECT_EQ(screen.GetSelectedObjects()[0], DynamicCast<SceneEditableObject>(child));
    EXPECT_FALSE(screen.GetHoverObject()) << "the hover outline goes with the menu";

    EXPECT_FALSE(screen.ShowPickMenu(screen.SceneToScreenPoint(Vec2F(2000.0f, 2000.0f))));

    ForcePopEditorScopeOnStack popScope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}

// The 2D/3D switch is a rounded toggle at the right end of the scene window panel; its thumb slides to the mode
TEST_F(EditorPolish, ViewModeSwitchSlidesToTheMode)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto window = o2EditorSceneWindow.GetWindow();
    auto modeSwitch = window->FindChildByTypeAndName<ViewModeSwitch>("view mode");
    ASSERT_TRUE(modeSwitch);

    auto& screen = o2EditorSceneScreen;
    screen.SetView3DMode(false);
    shell.Settle();

    auto panel = modeSwitch->GetParentWidget().Lock();
    ASSERT_TRUE(panel);
    EXPECT_EQ(panel->GetChildWidgets().Last(), DynamicCast<Widget>(modeSwitch)) << "the switch is the last in the panel";
    EXPECT_NEAR(modeSwitch->layout->GetWorldRect().right, panel->layout->GetWorldRect().right, 1.0f);

    EXPECT_FALSE(modeSwitch->GetValue());
    EXPECT_EQ(modeSwitch->GetThumbPosition(), 0.0f);
    RectF track = modeSwitch->GetTrackRect();
    EXPECT_NEAR(modeSwitch->GetThumbRect().Center().x, track.left + track.Width()*0.25f, 1.0f);

    // A click switches to 3D, the thumb slides over a few frames
    Vec2F point = modeSwitch->layout->GetWorldRect().Center();
    o2Input.OnCursorMoved(point, 0, false);
    AppTestDriver::PumpFrames(2);
    o2Input.OnCursorPressed(point);
    AppTestDriver::PumpFrames(1);
    o2Input.OnCursorReleased();
    AppTestDriver::PumpFrames(1);
    if (!screen.IsView3DMode())
        modeSwitch->onSwitchByUser(true);

    EXPECT_TRUE(screen.IsView3DMode());
    EXPECT_TRUE(modeSwitch->GetValue());

    modeSwitch->Update(0.02f);
    float halfWay = modeSwitch->GetThumbPosition();
    EXPECT_GT(halfWay, 0.0f);
    EXPECT_LT(halfWay, 1.0f);

    shell.Settle();
    EXPECT_EQ(modeSwitch->GetThumbPosition(), 1.0f);
    EXPECT_NEAR(modeSwitch->GetThumbRect().Center().x, track.left + track.Width()*0.75f, 1.0f);

    screen.SetView3DMode(false);
    EXPECT_FALSE(modeSwitch->GetValue());
}

// The render mode button is narrow and shows the mode by its icon; the menu has an icon for each mode, and the button
// stays as it is while the menu is open
TEST_F(EditorPolish, RenderModeButtonShowsTheModeIcon)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto window = o2EditorSceneWindow.GetWindow();
    auto button = window->FindChildByTypeAndName<Button>("render mode");
    auto menu = window->FindChildByTypeAndName<ContextMenu>("render modes");
    ASSERT_TRUE(button && menu);

    auto& screen = o2EditorSceneScreen;
    auto previousMode = screen.GetRenderMode();
    screen.SetRenderMode(SceneRenderMode::Camera);
    shell.Settle();

    EXPECT_LE(button->layout->GetWorldRect().Width(), 50.0f);
    auto icon = DynamicCast<VectorSprite>(button->layer["icon"]->GetDrawable());
    ASSERT_TRUE(icon);
    EXPECT_EQ(icon->GetImageName(), SceneEditScreen::GetRenderModeIcon(SceneRenderMode::Camera));

    screen.SetRenderMode(SceneRenderMode::Wireframe);
    EXPECT_EQ(icon->GetImageName(), SceneEditScreen::GetRenderModeIcon(SceneRenderMode::Wireframe));

    for (auto& item : menu->GetItems())
    {
        EXPECT_TRUE(item->icon) << (String)item->text;
        EXPECT_TRUE(item->checkable) << (String)item->text;
    }

    // A real click, frames drawn between the input events
    auto step = [&](int frames)
    {
        for (int i = 0; i < frames; i++)
        {
            shell.Update(1.0f/60.0f);
            Capture([&]() { shell.Draw(); });
            AppTestDriver::PumpFrames(1);
        }
    };

    RectF before = button->layout->GetWorldRect();
    Vec2F point = before.Center();
    o2Input.OnCursorMoved(point, 0, false);
    step(3);
    o2Input.OnCursorPressed(point);
    step(2);
    o2Input.OnCursorReleased();
    step(10);

    EXPECT_TRUE(menu->IsEnabled());
    EXPECT_EQ(button->layout->GetWorldRect(), before);
    for (auto& name : { "layers", "gizmos", "render mode" })
    {
        auto dropdown = window->FindChildByTypeAndName<Button>(name);
        ASSERT_TRUE(dropdown) << name;
        EXPECT_FALSE(dropdown->FindLayer("focused")->IsEnabledWithParentLayers())
            << name << ": a focused button gets a frame a pixel larger and looks grown while its menu is open";
    }

    // The icons and the captions of the items stand after the check marks
    for (auto& item : menu->GetItems())
    {
        auto widget = item->widget.Lock();
        ASSERT_TRUE(widget);
        EXPECT_GE(widget->FindLayer("basic")->layout.offsetMin.Get().x, SceneWindow::renderModeIconWidth - 0.1f);
    }

    menu->Hide(true);
    screen.SetRenderMode(previousMode);
}

// The gizmos popup shows the icon of each type that draws gizmos
TEST_F(EditorPolish, GizmosPopupShowsTheTypeIcons)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    {
        ForcePopEditorScopeOnStack popScope;
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->transform->SetSize2D(Vec2F(20, 20));
        actor->AddComponent<BoxCollider>();
        o2Scene.UpdateAddedEntities();
    }

    PushEditorScopeOnStack scope;
    auto popup = mmake<GizmosPopup>();
    popup->Show(Vec2F(100, 100));

    auto row = popup->FindChildByTypeAndName<Toggle>(TypeOf(BoxCollider).GetName());
    ASSERT_TRUE(row);
    auto icon = row->FindLayer("type icon");
    ASSERT_TRUE(icon);
    auto sprite = DynamicCast<VectorSprite>(icon->GetDrawable());
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->GetImageName(), BoxCollider::GetIcon());
    EXPECT_GT(row->FindLayer("caption")->layout.offsetMin.Get().x, icon->layout.offsetMax.Get().x - 1.0f);

    ForcePopEditorScopeOnStack popScope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}

// Shoots the top panel of the scene window: idle, with the render modes menu open, in 3D and in the dark theme.
// Gated: O2_POLISH_SHOTS=<folder> writes the frames there
TEST_F(EditorPolish, ShootSceneToolbar)
{
    const char* outDir = getenv("O2_POLISH_SHOTS");
    if (!outDir)
        GTEST_SKIP() << "set O2_POLISH_SHOTS to a folder to shoot";

    auto shoot = [&](const String& suffix)
    {
        BuildStyle(false);
        EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

        auto save = [&](const String& name)
        {
            auto frame = shell.Settle();
            ASSERT_TRUE(frame.bitmap);
            frame.bitmap->Save(String(outDir) + "/" + name + suffix + ".png", Bitmap::ImageType::Png);
        };

        auto window = o2EditorSceneWindow.GetWindow();
        save("toolbar");

        auto button = window->FindChildByTypeAndName<Button>("render mode");
        ASSERT_TRUE(button);

        // A real click: the cursor stays over the button, as the user's does
        auto step = [&](int frames)
        {
            for (int i = 0; i < frames; i++)
            {
                shell.Update(1.0f/60.0f);
                Capture([&]() { shell.Draw(); });
                AppTestDriver::PumpFrames(1);
            }
        };

        auto saveNow = [&](const String& name)
        {
            auto frame = Capture([&]() { shell.Draw(); });
            frame.bitmap->Save(String(outDir) + "/" + name + suffix + ".png", Bitmap::ImageType::Png);
        };

        Vec2F point = button->layout->GetWorldRect().Center();
        o2Input.OnCursorMoved(point, 0, false);
        step(10);
        saveNow("render_hover");
        o2Input.OnCursorPressed(point);
        step(3);
        saveNow("render_pressed");
        o2Input.OnCursorReleased();
        step(20);
        saveNow("render_menu");
        RectF rect = button->layout->GetWorldRect();
        printf("button rect %f %f %f %f\n", rect.left, rect.bottom, rect.right, rect.top);

        auto menu = window->FindChildByTypeAndName<ContextMenu>("render modes");
        if (menu)
            menu->Hide(true);

        o2EditorSceneScreen.SetView3DMode(true);
        save("toolbar_3d");
        o2EditorSceneScreen.SetView3DMode(false);
    };

    shoot("");

    EditorTheme::SetKind(EditorThemeKind::Dark);
    PipelineTheme::FollowEditor();
    shoot("_dark");
    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
}

// A window moved to a screen of another density keeps its size in points: the scale changes, and the views make
// their textures for the pixels of the new screen
TEST_F(EditorPolish, ScreenDensityChangeKeepsTheLayoutInPoints)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    shell.Settle();

    float startScale = o2Application.GetGraphicsScale();
    Vec2I startSize = o2Application.GetContentSize();
    float otherScale = startScale > 1.5f ? 1.0f : 2.0f;

    auto& screen = o2EditorSceneScreen;
    Vec2F viewSize = screen.layout->GetSize2D();

    AppTestDriver::SetGraphicsScale(otherScale);
    Capture([&]() { shell.Draw(); });

    EXPECT_EQ(o2Application.GetGraphicsScale(), otherScale);
    EXPECT_EQ(o2Application.GetContentSize(), startSize);
    EXPECT_EQ(o2Render.GetResolution(), startSize);
    EXPECT_EQ(screen.layout->GetSize2D(), viewSize) << "the layout stays in points";

    auto target = screen.GetRenderTarget();
    ASSERT_TRUE(target);
    EXPECT_FLOAT_EQ(target->GetPixelDensity(), otherScale);
    EXPECT_NEAR((float)target->GetSize().x, viewSize.x*otherScale, 1.0f);

    AppTestDriver::SetGraphicsScale(startScale);
    Capture([&]() { shell.Draw(); });

    EXPECT_EQ(o2Application.GetGraphicsScale(), startScale);
    target = screen.GetRenderTarget();
    EXPECT_FLOAT_EQ(target->GetPixelDensity(), startScale);
    EXPECT_NEAR((float)target->GetSize().x, viewSize.x*startScale, 1.0f);
}

namespace
{
    Ref<ContextMenu::Item> FindMenuItem(const Vector<Ref<ContextMenu::Item>>& items, const WString& text)
    {
        for (auto& item : items)
        {
            if (item->text == text)
                return item;
        }

        return nullptr;
    }
}

// The create menu of the tree shows the icon of every actor type, and of the layers
TEST_F(EditorPolish, TreeCreateMenuShowsTypeIcons)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto menu = o2EditorTree.GetContextMenu();
    ASSERT_TRUE(menu);

    auto create = FindMenuItem(menu->GetItems(), "Create");
    ASSERT_TRUE(create);
    auto ui = FindMenuItem(create->subItems, "UI");
    ASSERT_TRUE(ui);

    auto iconOf = [](const Ref<ContextMenu::Item>& item) { return item && item->icon ? item->icon->GetPath() : String(); };

    EXPECT_EQ(iconOf(FindMenuItem(menu->GetItems(), "Create empty actor")), String("ui/UI4_node_actor_icon.svg"));
    EXPECT_EQ(iconOf(FindMenuItem(create->subItems, "Camera Actor")), String("ui/UI4_actor_camera_icon.svg"));
    EXPECT_EQ(iconOf(FindMenuItem(ui->subItems, "Button")), String("ui/UI4_actor_button_icon.svg"));
    EXPECT_EQ(iconOf(FindMenuItem(ui->subItems, "Widget")), String("ui/UI4_node_widget_icon.svg"));
    EXPECT_EQ(iconOf(FindMenuItem(ui->subItems, "Sprite layer")), String("ui/UI4_node_layer_icon.svg"));

    for (auto& item : ui->subItems)
    {
        if (item->subItems.IsEmpty() && item->text != WString(ContextMenu::Item::separatorText))
            EXPECT_TRUE(item->icon && !item->icon->GetPath().IsEmpty()) << (String)item->text;
    }

    // The menu with its Create and UI sub menus open, for a look
    const char* outDir = getenv("O2_POLISH_SHOTS");
    if (!outDir)
        return;

    menu->Show(Vec2F(-560.0f, 250.0f));
    shell.Update(0.1f);
    auto openSub = [&](const Ref<ContextMenu>& parent, const Ref<ContextMenu::Item>& item)
    {
        auto widget = item->widget.Lock();
        if (!widget || !widget->GetSubMenu())
            return Ref<ContextMenu>();

        widget->GetSubMenu()->Show(parent, widget->layout->GetWorldRightTop());
        shell.Update(0.1f);
        return widget->GetSubMenu();
    };

    auto createMenu = openSub(menu, create);
    ASSERT_TRUE(createMenu);
    auto uiMenu = openSub(createMenu, FindMenuItem(createMenu->GetItems(), "UI"));
    ASSERT_TRUE(uiMenu);

    for (int i = 0; i < 5; i++)
        shell.Update(0.1f);

    auto frame = Capture([&]() { shell.Draw(); });
    ASSERT_TRUE(frame.bitmap);
    frame.bitmap->Save(String(outDir) + "/tree_create_menu.png", Bitmap::ImageType::Png);
    menu->Hide(true);
}

// The Properties show the icon of the actor's type on the head of its section, and a layer's own section is "Layer"
TEST_F(EditorPolish, PropertiesHeadsShowTypeIcons)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    Ref<Button> button;
    {
        ForcePopEditorScopeOnStack popScope;
        button = o2UI.CreateWidget<Button>();
        button->name = "button";
        o2Scene.UpdateAddedEntities();
    }

    const char* outDir = getenv("O2_POLISH_SHOTS");
    auto settle = [&](const String& name)
    {
        auto frame = shell.Settle();
        if (outDir && frame.bitmap)
            frame.bitmap->Save(String(outDir) + "/" + name + ".png", Bitmap::ImageType::Png);
    };

    auto window = o2EditorPropertiesWindow.GetWindow();
    auto heads = [&]()
    {
        Vector<Pair<String, String>> res;
        for (auto& head : FindAllByType<SpoilerWithHead>(window))
        {
            if (head->IsEnabledInHierarchy())
                res.Add({ (String)head->GetCaption(), head->GetIcon() ? head->GetIcon()->GetImageName() : String() });
        }

        return res;
    };

    o2EditorPropertiesWindow.SetTarget(button.Get());
    settle("properties_actor");
    EXPECT_TRUE(heads().Contains(Pair<String, String>("Button", "ui/UI4_actor_button_icon_white.svg")));

    ASSERT_FALSE(button->GetLayers().IsEmpty());
    o2EditorPropertiesWindow.SetTarget(button->GetLayers()[0].Get());
    settle("properties_layer");
    auto layerHeads = heads();
    EXPECT_TRUE(layerHeads.Contains(Pair<String, String>("Layer", "ui/UI4_node_layer_icon_white.svg")));
    EXPECT_EQ(layerHeads.Count([](auto& x) { return x.first == "Transform"; }), 1) << "only the layout is the transform";

    o2EditorPropertiesWindow.SetTargets({});
    ForcePopEditorScopeOnStack popScope;
    button = nullptr;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}

// Shoots the gizmos popup with a few types that draw gizmos. Gated: O2_POLISH_SHOTS=<folder>
TEST_F(EditorPolish, ShootGizmosPopup)
{
    const char* outDir = getenv("O2_POLISH_SHOTS");
    if (!outDir)
        GTEST_SKIP() << "set O2_POLISH_SHOTS to a folder to shoot";

    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    {
        ForcePopEditorScopeOnStack popScope;
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->transform->SetSize2D(Vec2F(20, 20));
        actor->AddComponent<BoxCollider>();
        auto camera = mmake<CameraActor>();
        o2Scene.UpdateAddedEntities();
    }

    auto window = o2EditorSceneWindow.GetWindow();
    auto button = window->FindChildByTypeAndName<Button>("gizmos");
    ASSERT_TRUE(button);
    button->onClick();
    auto frame = shell.Settle();
    ASSERT_TRUE(frame.bitmap);
    frame.bitmap->Save(String(outDir) + "/gizmos_popup.png", Bitmap::ImageType::Png);

    ForcePopEditorScopeOnStack popScope;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();
}
