#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"

#include "o2/Utils/System/Clipboard.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/AssetsLineDlg.h"
#include <thread>

// The Pipeline settings window inside the whole editor: its menus, windows and their shortcuts listen to the keys too

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    // A frame as the application runs it: the editor updated and drawn, then the input and the events
    void Step(EditorShell& shell, int frames = 1)
    {
        for (int i = 0; i < frames; i++)
        {
            {
                PushEditorScopeOnStack scope;
                o2Events.PostUpdate();
                shell.Update(1.0f/60.0f);
                o2Render.Begin();
                o2Render.SetCamera(Camera());
                shell.Draw();
                o2UI.DrawCurrentLayerTopWidgets();
                o2Render.End();
            }
            AppTestDriver::PumpFrames(1);
        }
    }

    // A dialog of its own for every test, destroyed after it, and a sync that asks no server. The sync lives as long as
    // the process, as the editor's does; the dialog scope opens after the set up, so it takes no singleton of the fixture
    struct AssetsLineDlgShell : EditorVectorUIFixture
    {
        std::unique_ptr<SingletonsScope<AssetsLineDlg>> dialogScope;

        void SetUp() override
        {
            EditorVectorUIFixture::SetUp();

            static Ref<AssetsLineSync> sync = AssetsLineSync::IsSingletonInitialzed() ? Ref(AssetsLineSync::InstancePtr()) :
                mmake<AssetsLineSync>();
            sync->rebuildAssets = []() {};
            sync->openUrl = [](const String&) {};

            dialogScope = std::make_unique<SingletonsScope<AssetsLineDlg>>();
            Singleton<AssetsLineDlg>::mInstance = nullptr;
        }

        void TearDown() override
        {
            dialogScope = nullptr;
            EditorVectorUIFixture::TearDown();
        }
    };

    void Press(EditorShell& shell, KeyboardKey key, bool down)
    {
        if (down)
            o2Input.OnKeyPressed(key);
        else
            o2Input.OnKeyReleased(key);

        Step(shell);
    }
}

TEST_F(AssetsLineDlgShell, TheServerAddressPastesInTheEditor)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
    Step(shell, 10);

    AssetsLineDlg::Show();
    shell.RaiseDialogs();
    Step(shell, 5);

    auto window = AssetsLineDlg::Instance().GetWindow();
    auto server = DynamicCast<EditBox>(window->FindChild("server"));
    ASSERT_TRUE(server);

    Vec2F point = server->layout->GetWorldRect().Center();
    o2Input.OnCursorMoved(point);
    Step(shell);
    o2Input.OnCursorPressed(point);
    Step(shell);
    o2Input.OnCursorReleased();
    Step(shell, 2);
    ASSERT_TRUE(server->IsFocused()) << "focused: " << (o2UI.GetFocusedWidget() ? o2UI.GetFocusedWidget()->GetName() : String("none"));

#if defined PLATFORM_MAC
    const KeyboardKey command = VK_COMMAND;
#else
    const KeyboardKey command = VK_CONTROL;
#endif

    // Select all, then paste over it
    WString saved = Clipboard::GetText();
    Clipboard::SetText("https://example.test");
    Press(shell, command, true);
    Press(shell, VK_A, true);
    Press(shell, VK_A, false);
    Press(shell, VK_V, true);
    Press(shell, VK_V, false);
    Press(shell, command, false);
    Clipboard::SetText(saved);

    EXPECT_TRUE(server->IsFocused()) << "focused: " << (o2UI.GetFocusedWidget() ? o2UI.GetFocusedWidget()->GetName() : String("none"));
    EXPECT_EQ((String)server->GetText(), "https://example.test");

    window->Hide(true);
}

// In the dark theme every text of the window reads on its panels: titles, values and hints
TEST_F(AssetsLineDlgShell, TextsReadInTheDarkTheme)
{
    EditorTheme::SetKind(EditorThemeKind::Dark);
    PipelineTheme::FollowEditor();
    BuildStyle(false);
    {
        SingletonsScope<AssetsLineDlg> dialogScope;
        Singleton<AssetsLineDlg>::mInstance = nullptr;
        EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
        AssetsLineDlg::Show();

        auto luminance = [](const Color4& c) { return 0.2126f*c.r + 0.7152f*c.g + 0.0722f*c.b; };
        Vector<Ref<Actor>> actors;
        AssetsLineDlg::Instance().GetWindow()->GetAllChildrenActors(actors);
        int labels = 0;
        for (auto& actor : actors)
        {
            if (auto label = DynamicCast<Label>(actor))
            {
                labels++;
                EXPECT_GT(luminance(label->GetColor()), 120.0f) << (String)label->GetText();
            }
        }
        EXPECT_GT(labels, 10);

        AssetsLineDlg::Instance().GetWindow()->Hide(true);
    }

    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
    BuildStyle(false);
}

// The window in both themes, for review (O2_EDITOR_THEME_OUT=<folder>)
TEST_F(AssetsLineDlgShell, Gallery)
{
    const char* out = getenv("O2_EDITOR_THEME_OUT");
    if (!out)
        GTEST_SKIP() << "O2_EDITOR_THEME_OUT is not set";

    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        PipelineTheme::FollowEditor();
        BuildStyle(false);

        SingletonsScope<AssetsLineDlg> dialogScope;
        Singleton<AssetsLineDlg>::mInstance = nullptr;
        EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
        String theme = kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png";
        AssetsLineDlg::Show(AssetsLineDlg::Page::Sync);
        shell.RaiseDialogs();
        Frame frame = shell.Settle();
        frame.bitmap->Save(String(out) + "/assetsline_dlg" + theme, Bitmap::ImageType::Png);

        // The local agents with their real states: the checks run the agents' own status commands
        AssetsLineDlg::Show(AssetsLineDlg::Page::Agents);
        for (int i = 0; i < 300; i++)
        {
            AssetsLineDlg::Instance().Update(0.02f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        shell.RaiseDialogs();
        frame = shell.Settle();
        frame.bitmap->Save(String(out) + "/assetsline_dlg_agents" + theme, Bitmap::ImageType::Png);
        AssetsLineDlg::Instance().ShowPage(AssetsLineDlg::Page::Sync);
        AssetsLineDlg::Instance().GetWindow()->Hide(true);
    }

    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
}

// An assets rebuild (the pipeline autosave and the sync make one every few seconds) leaves a field being typed in focused: the
// assets window took the focus at every rebuild
TEST_F(AssetsLineDlgShell, AnAssetsRebuildKeepsTheFocus)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
    Step(shell, 10);

    AssetsLineDlg::Show(AssetsLineDlg::Page::Sync);
    shell.RaiseDialogs();
    Step(shell, 5);
    auto window = AssetsLineDlg::Instance().GetWindow();
    auto server = DynamicCast<EditBox>(window->FindChild("server"));
    ASSERT_TRUE(server);
    Vec2F point = server->layout->GetWorldRect().Center();
    o2Input.OnCursorMoved(point);
    Step(shell);
    o2Input.OnCursorPressed(point);
    Step(shell);
    o2Input.OnCursorReleased();
    Step(shell, 2);
    ASSERT_TRUE(server->IsFocused());

    // A rebuild for a pipeline file: nothing of the editor takes the field's focus
    o2Assets.onAssetsRebuilt(Vector<UID>());
    Step(shell, 3);
    EXPECT_TRUE(server->IsFocused()) << "focused: " << (o2UI.GetFocusedWidget() ? o2UI.GetFocusedWidget()->GetName() : String("none"));

    window->Hide(true);
}

// A button or a tab under the cursor keeps its size and its layers' sizes, on every page and in both themes
// (O2_HOVER_OUT=<folder> also saves each one before and under the cursor)
TEST_F(AssetsLineDlgShell, HoveringKeepsTheButtonsWidth)
{
    const char* out = getenv("O2_HOVER_OUT");
    auto layerRects = [](const Ref<Widget>& widget)
    {
        Vector<Pair<String, RectF>> res;
        Function<void(const Ref<WidgetLayer>&)> add = [&](const Ref<WidgetLayer>& layer)
        {
            if (layer->GetDrawable() && layer->transparency > 0.01f)
                res.Add({ layer->name, layer->GetRect() });
            for (auto& child : layer->GetChildren())
                add(child);
        };
        for (auto& layer : widget->GetLayers())
            add(layer);
        return res;
    };
    auto crop = [](const Ref<Bitmap>& frame, const RectF& world, const String& path)
    {
        Vec2I size = frame->GetSize();
        int left = Math::Clamp((int)(world.left + size.x*0.5f) - 8, 0, size.x - 1);
        int right = Math::Clamp((int)(world.right + size.x*0.5f) + 8, 0, size.x);
        int top = Math::Clamp((int)(size.y*0.5f - world.top) - 8, 0, size.y - 1);
        int bottom = Math::Clamp((int)(size.y*0.5f - world.bottom) + 8, 0, size.y);
        Bitmap piece(frame->GetFormat(), Vec2I(right - left, bottom - top));
        piece.CopyImage(*frame, Vec2I(), RectI(left, top, right, bottom));
        piece.Save(path, Bitmap::ImageType::Png);
    };

    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        PipelineTheme::FollowEditor();
        // O2_HOVER_SAVED_STYLE=1 takes the saved style, as the editor loads it
        if (getenv("O2_HOVER_SAVED_STYLE"))
            ASSERT_TRUE(o2UI.LoadStyle(EditorTheme::StylesFolder()));
        else
            BuildStyle(false);
        String theme = kind == EditorThemeKind::Dark ? "dark" : "light";
        {
            SingletonsScope<AssetsLineDlg> scope;
            Singleton<AssetsLineDlg>::mInstance = nullptr;
            EditorShell shell(MakeLayout("pipeline window", "assets window", "properties window"));
            Step(shell, 10);

            int checked = 0;
            for (auto page : { AssetsLineDlg::Page::Sync, AssetsLineDlg::Page::Keys, AssetsLineDlg::Page::Agents,
                               AssetsLineDlg::Page::Editor })
            {
                AssetsLineDlg::Show(page);
                shell.RaiseDialogs();
                for (int i = 0; i < 20; i++)
                {
                    AssetsLineDlg::Instance().Update(0.05f);
                    Step(shell);
                }

                // The parts other states show: connected, the token way, an agent to set up, the custom paths
                auto window = AssetsLineDlg::Instance().GetWindow();
                for (auto name : { "linked", "token panel", "claude setup", "codex setup", "local agent custom section" })
                {
                    if (auto part = window->FindChild(name))
                        part->enabled = true;
                }
                if (page == AssetsLineDlg::Page::Sync)
                {
                    if (auto part = window->FindChild("connect"))
                        part->enabled = true;
                }
                Step(shell, 5);
                Vector<Ref<Actor>> actors;
                window->GetAllChildrenActors(actors);
                for (auto& actor : actors)
                {
                    auto widget = DynamicCast<Widget>(actor);
                    if (!widget || !widget->IsEnabledInHierarchy() || (!DynamicCast<Button>(widget) && !DynamicCast<Toggle>(widget)))
                        continue;
                    if (widget->GetName() == "closeButton" || widget->GetName() == "optionsButton")
                        continue;

                    RectF rect = widget->layout->GetWorldRect();
                    if (rect.Width() < 1 || !window->layout->GetWorldRect().IsInside(rect.Center()))
                        continue;

                    o2Input.OnCursorMoved(Vec2F(-2000, -2000));
                    Step(shell, 15);
                    auto layersBefore = layerRects(widget);
                    Frame before = Capture([&]() { shell.Draw(); });

                    o2Input.OnCursorMoved(rect.Center());
                    Step(shell, 15);
                    RectF hovered = widget->layout->GetWorldRect();
                    auto layersHovered = layerRects(widget);
                    Frame under = Capture([&]() { shell.Draw(); });

                    String name = widget->GetName();
                    EXPECT_NEAR(hovered.Width(), rect.Width(), 0.5f) << theme << ": " << name;
                    EXPECT_NEAR(hovered.Height(), rect.Height(), 0.5f) << theme << ": " << name;
                    for (auto& layer : layersHovered)
                    {
                        for (auto& was : layersBefore)
                        {
                            if (was.first == layer.first)
                                EXPECT_NEAR(layer.second.Width(), was.second.Width(), 0.5f) << theme << ": " << name << " layer " << layer.first;
                        }
                    }

                    if (out)
                    {
                        String clean = name;
                        clean.ReplaceAll(" ", "_");
                        clean.ReplaceAll(".", "");
                        String file = String(out) + "/" + theme + "_" + (String)(int)page + "_" + clean;
                        crop(before.bitmap, rect, file + "_0.png");
                        crop(under.bitmap, rect, file + "_1.png");
                    }
                    checked++;
                }
            }
            EXPECT_GT(checked, 15);

            o2Input.OnCursorMoved(Vec2F(-2000, -2000));
            AssetsLineDlg::Instance().ShowPage(AssetsLineDlg::Page::Sync);
            AssetsLineDlg::Instance().GetWindow()->Hide(true);
        }
    }

    EditorTheme::SetKind(EditorThemeKind::Light);
    PipelineTheme::FollowEditor();
    BuildStyle(false);
}

// Only a rebuild that changed a picture, an atlas or a font reloads the textures, the atlases and the sprites (reloading them
// all blanked the editor's icons for a moment at every pipeline autosave)
TEST(RenderAssetsRebuild, OnlyPicturesAtlasesAndFontsReloadTheTextures)
{
    auto info = [](const String& path, const Ref<AssetMeta>& meta)
    {
        AssetInfo res(meta);
        res.path = path;
        return res;
    };
    EXPECT_FALSE(Render::IsRenderAsset(info("Pipelines/Test.pipeline", mmake<AssetMeta>()))) << "a pipeline is data";
    EXPECT_FALSE(Render::IsRenderAsset(info("Scripts/Game.js", mmake<AssetMeta>())));
    EXPECT_TRUE(Render::IsRenderAsset(info("Sprites/tile.png", mmake<AssetMeta>()))) << "by its extension";
    EXPECT_TRUE(Render::IsRenderAsset(info("Sprites/tile.png", mmake<ImageAsset::Meta>()))) << "by its meta";
    EXPECT_TRUE(Render::IsRenderAsset(info("UI/icon.svg", mmake<AssetMeta>())));
    EXPECT_TRUE(Render::IsRenderAsset(AssetInfo::empty)) << "gone from the tree: reload to be safe";
    EXPECT_FALSE(Render::ChangesRenderResources({})) << "nothing changed";
}
