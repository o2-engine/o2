#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"

#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2/Utils/Math/Spline.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/UI/CurveEditor/CurvesEditor.h"
#include "o2Editor/UI/SplineEditor/SplineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

// The two editor themes as the editor starts with them, and a gallery of both for the report (O2_EDITOR_THEME_OUT=<folder>)

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    // Puts the light theme back whatever the test did
    struct LightAfter
    {
        ~LightAfter()
        {
            EditorTheme::SetKind(EditorThemeKind::Light);
            PipelineTheme::FollowEditor();
        }
    };

    float MeanLuminance(const Bitmap& bitmap)
    {
        const UInt8* data = bitmap.GetData();
        size_t pixels = (size_t)bitmap.GetSize().x*bitmap.GetSize().y;
        double sum = 0;
        for (size_t i = 0; i < pixels; i++)
            sum += 0.2126*data[i*4] + 0.7152*data[i*4 + 1] + 0.0722*data[i*4 + 2];

        return (float)(sum/(double)pixels);
    }

    // Share of the pixels whose channels differ by more than the tolerance
    float DifferentShare(const Bitmap& a, const Bitmap& b, int tolerance = 2)
    {
        if (a.GetSize() != b.GetSize())
            return 1.0f;

        const UInt8* da = a.GetData();
        const UInt8* db = b.GetData();
        size_t pixels = (size_t)a.GetSize().x*a.GetSize().y;
        size_t different = 0;
        for (size_t i = 0; i < pixels; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                if (Math::Abs((int)da[i*4 + c] - (int)db[i*4 + c]) > tolerance)
                {
                    different++;
                    break;
                }
            }
        }

        return (float)different/(float)pixels;
    }

    class EditorThemeSwitch : public EditorVectorUIFixture
    {
    public:
        void SetUp() override
        {
            EditorTheme::SetKind(EditorThemeKind::Light);
            PipelineTheme::FollowEditor();
            EditorVectorUIFixture::SetUp();
        }

        // Shoots the editor started in a theme: the art, the styles and the windows of that theme
        Frame ShootEditor(EditorThemeKind kind, const WindowsLayout& layout, TinyScene& scene)
        {
            EditorTheme::SetKind(kind);
            BuildStyle(false);

            EditorShell shell(layout);
            o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ scene.root->GetID() });
            return shell.Settle();
        }
    };
}

// The light look is the o2 look as it was, also after the dark art and styles were loaded in the same process
TEST_F(EditorThemeSwitch, LightLookStaysTheSame)
{
    LightAfter lightAfter;
    TinyScene scene;
    auto layout = MakeLayout("scene window", "assets window", "properties window");

    Frame light = ShootEditor(EditorThemeKind::Light, layout, scene);
    Frame dark = ShootEditor(EditorThemeKind::Dark, layout, scene);
    Frame lightAgain = ShootEditor(EditorThemeKind::Light, layout, scene);
    ASSERT_TRUE(light.bitmap && dark.bitmap && lightAgain.bitmap);

    EXPECT_GT(MeanLuminance(*light.bitmap), 150.0f);
    EXPECT_LT(MeanLuminance(*dark.bitmap), 60.0f);
    EXPECT_EQ(PipelineTheme::GetKind(), PipelineThemeKind::Light);

    float different = DifferentShare(*light.bitmap, *lightAgain.bitmap);
    printf("[theme] light frame after the dark one: %.4f%% of the pixels differ\n", different*100.0f);
    EXPECT_LE(different, 0.001f) << "the light look differs after the dark theme was loaded";
}

// Every style of both themes is saved readable: a NaN is written as an empty number, and the editor would load a theme
// with styles missing
TEST_F(EditorThemeSwitch, StylesSaveReadable)
{
    LightAfter lightAfter;

    for (auto kind : { EditorThemeKind::Dark, EditorThemeKind::Light })
    {
        EditorTheme::SetKind(kind);
        BuildStyle(false);

        PushEditorScopeOnStack scope;
        int unreadable = 0;
        for (auto& style : o2UI.GetWidgetStyles())
        {
            DataDocument doc;
            doc = style->GetActor();

            DataDocument readBack;
            Ref<Actor> actor;
            if (readBack.LoadFromData(doc.SaveAsString()))
                readBack.Get(actor);

            if (!actor)
            {
                unreadable++;
                ADD_FAILURE() << EditorTheme::NameOf(kind) << ": style " << style->GetActor()->GetType().GetName()
                              << " '" << style->GetActor()->GetName() << "' is saved unreadable";
            }
            else
                actor->RemoveFromScene();
        }
        EXPECT_EQ(unreadable, 0) << EditorTheme::NameOf(kind);
    }

    BuildStyle(false);
}

// The images of the project keep their colours in the dark theme: only the editor art is restyled
TEST_F(EditorThemeSwitch, ProjectArtKeepsItsColors)
{
    LightAfter lightAfter;

    EditorTheme::SetKind(EditorThemeKind::Dark);

    VectorImage image;
    image.shapes.Add(VectorShape());
    image.shapes[0].AddRect(Vec2F(0, 0), Vec2F(10, 10));
    image.shapes[0].fill = VectorPaint::Solid(Color4(0, 156, 141));

    VectorImage projectImage = image;
    VectorImageAsset::ImageRemap()(projectImage, o2Assets.GetAssetsTree().builtAssetsPath + "ui/UI4_Background.svg");
    EXPECT_EQ(projectImage.shapes[0].fill.color, Color4(0, 156, 141));

    auto editorAsset = AssetRef<VectorImageAsset>("ui/UI4_Background.svg");
    ASSERT_TRUE(editorAsset);
    ASSERT_TRUE(editorAsset->IsEditorAsset());

    VectorImage editorImage = image;
    VectorImageAsset::ImageRemap()(editorImage, editorAsset->GetBuiltFullPath());
    EXPECT_NE(editorImage.shapes[0].fill.color, Color4(0, 156, 141));
}

// The style colours of the light look and their dark roles
TEST_F(EditorThemeSwitch, StyleColorsTakeTheirDarkRoles)
{
    LightAfter lightAfter;

    EXPECT_EQ(EditorTheme::StyleColor(Color4(96, 125, 139)), Color4(96, 125, 139));

    EditorTheme::SetKind(EditorThemeKind::Dark);
    auto& dark = EditorTheme::Get();
    EXPECT_EQ(EditorTheme::StyleColor(Color4(96, 125, 139)), dark.text);
    EXPECT_EQ(EditorTheme::StyleColor(Color4(44, 62, 80)), dark.text);
    EXPECT_EQ(EditorTheme::StyleColor(Color4(0, 156, 141)), dark.accent);
    EXPECT_EQ(EditorTheme::StyleColor(Color4(225, 232, 232)), dark.viewBack);
    EXPECT_EQ(EditorTheme::StyleColor(Color4(12, 34, 56)), Color4(12, 34, 56));
    EXPECT_EQ(EditorTheme::StyleColor(Color4(0, 0, 0, 20)), Color4(255, 255, 255, 10));
    EXPECT_EQ(EditorTheme::StyleColor(Color4::Black()), dark.text);
}

// What the styles draw in plain colours reads on the dark panels: the carets of the fields, the track of the state bar
TEST_F(EditorThemeSwitch, DarkStylesKeepCaretsAndTracksVisible)
{
    LightAfter lightAfter;
    EditorTheme::SetKind(EditorThemeKind::Dark);
    BuildStyle(false);

    auto luminance = [](const Color4& c) { return 0.2126f*c.r + 0.7152f*c.g + 0.0722f*c.b; };

    int carets = 0;
    for (auto& style : o2UI.GetWidgetStyles())
    {
        auto actor = style->GetActor();
        if (!actor)
            continue;

        Vector<Ref<Actor>> actors = { actor };
        actor->GetAllChildrenActors(actors);
        for (auto& child : actors)
        {
            auto box = DynamicCast<EditBox>(child);
            if (!box || !box->GetCaretDrawable())
                continue;

            carets++;
            EXPECT_GT(luminance(box->GetCaretDrawable()->GetColor()), 150.0f) << style->GetActor()->GetName();
        }
    }
    EXPECT_GT(carets, 0);

    auto bar = DynamicCast<Widget>(o2UI.GetWidgetStyle<HorizontalProgress>("animation state bar"));
    ASSERT_TRUE(bar);
    auto track = bar->GetLayer("back/sprite");
    ASSERT_TRUE(track && track->GetDrawable());
    EXPECT_LT(luminance(track->GetDrawable()->GetColor()), 110.0f);
}

// --- Gallery for the report ------------------------------------------------------------------------------------------

namespace
{
    struct GalleryItem
    {
        Ref<Widget> sample;
        String      state;
        bool        value = false;
    };

    const Vec2I galleryCell(170, 76);
    const float galleryCellMargin = 6.0f;

    // Every style in its default look and with each of its states turned over
    Vector<GalleryItem> CollectGalleryItems()
    {
        Vector<GalleryItem> res;
        for (auto& style : o2UI.GetWidgetStyles())
        {
            auto sample = DynamicCast<Widget>(style->GetActor());
            if (!sample || sample->GetType() == TypeOf(AnimationTree))
                continue;

            res.Add({ sample, "", false });
            for (auto& state : sample->GetStates())
            {
                if (state->name != "visible")
                    res.Add({ sample, state->name, !state->GetState() });
            }
        }
        return res;
    }

    Frame RenderGalleryPage(const Vector<GalleryItem>& items, int begin, int end, const Color4& background)
    {
        PushEditorScopeOnStack scope;

        auto page = mmake<Widget>();
        *page->layout = WidgetLayout::BothStretch();
        EditorUIRoot.AddWidget(page);

        int columns = frameSize.x/galleryCell.x;
        Vector<Ref<Widget>> widgets;
        for (int i = begin; i < end; i++)
        {
            Vec2F cellPosition((float)((i - begin)%columns*galleryCell.x), (float)(-(i - begin)/columns*galleryCell.y));
            auto widget = items[i].sample->CloneAsRef<Widget>();
            *widget->layout = WidgetLayout::Based(BaseCorner::LeftTop,
                                                  (Vec2F)galleryCell - Vec2F(galleryCellMargin, galleryCellMargin)*2.0f,
                                                  cellPosition + Vec2F(galleryCellMargin, -galleryCellMargin));
            page->AddChild(widget);
            widgets.Add(widget);
        }

        page->SetEnabledForcible(false);
        page->SetEnabledForcible(true);
        for (auto& widget : widgets)
            widget->SetEnabledForcible(true);

        ParkCursor();
        for (int i = 0; i < settleSteps/2; i++)
            UpdateRoot(settleStep);

        for (int i = begin; i < end; i++)
        {
            if (!items[i].state.IsEmpty())
                widgets[i - begin]->SetStateForcible(items[i].state, items[i].value);
        }

        Frame res = Capture([&]() {
            o2Render.Clear(background);
            page->Draw();
        });

        EditorUIRoot.RemoveWidget(page);
        return res;
    }

    // A spline edited in screen space: its local space is the screen's
    struct ScreenSplineWrapper : SplineEditor::ISplineWrapper
    {
        Ref<Spline> spline;

        int GetPointsCount() const override { return spline->GetKeys().Count(); }
        bool IsClosed() const override { return spline->IsClosed(); }
        void AddPoint(int idx, const Vec2F& position, const Vec2F& prevSupport, const Vec2F& nextSupport) override
        {
            spline->InsertKey(idx, position, 0.0f, prevSupport, nextSupport);
        }
        void RemovePoint(int idx) override { spline->RemoveKey(idx); }
        Vec2F GetPointPos(int idx) const override { return spline->GetKey(idx).value; }
        void SetPointPos(int idx, const Vec2F& pos) override { auto key = spline->GetKey(idx); key.value = pos; spline->SetKey(key, idx); }
        void SetPointRangeValue(int idx, float value) override { auto key = spline->GetKey(idx); key.valueRange = value; spline->SetKey(key, idx); }
        float GetPointRangeValue(int idx) const override { return spline->GetKey(idx).valueRange; }
        Vec2F GetPointPrevSupportPos(int idx) const override { auto key = spline->GetKey(idx); return key.prevSupport + key.value; }
        void SetPointPrevSupportPos(int idx, const Vec2F& pos) override { auto key = spline->GetKey(idx); key.prevSupport = pos - key.value; spline->SetKey(key, idx); }
        Vec2F GetPointNextSupportPos(int idx) const override { auto key = spline->GetKey(idx); return key.nextSupport + key.value; }
        void SetPointNextSupportPos(int idx, const Vec2F& pos) override { auto key = spline->GetKey(idx); key.nextSupport = pos - key.value; spline->SetKey(key, idx); }
        const ApproximationVec2F* GetPointApproximationLeft(int idx) const override { return spline->GetKeys()[idx].GetApproximatedPointsLeft(); }
        const ApproximationVec2F* GetPointApproximationRight(int idx) const override { return spline->GetKeys()[idx].GetApproximatedPointsRight(); }
        int GetPointApproximationCount(int idx) const override { return spline->GetKeys()[idx].GetApproximatedPointsCount(); }
    };

    class EditorThemeGallery : public EditorThemeSwitch
    {
    public:
        String outDir;

        void SetUp() override
        {
            const char* out = getenv("O2_EDITOR_THEME_OUT");
            if (!out)
                GTEST_SKIP() << "O2_EDITOR_THEME_OUT is not set";

            outDir = String(out) + "/";
            o2FileSystem.FolderCreate(outDir, true);
            EditorThemeSwitch::SetUp();
        }

        // A skipped gallery has set nothing up
        void TearDown() override
        {
            if (!outDir.IsEmpty())
                EditorThemeSwitch::TearDown();
        }

        // The scenario in the editor started in the light theme, then in the one started in the dark theme
        void Shoot(const WindowsLayout& layout, const Function<void(EditorShell&, Vector<Frame>&)>& scenario,
                   const Vector<String>& names)
        {
            LightAfter lightAfter;
            for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
            {
                EditorTheme::SetKind(kind);
                BuildStyle(false);

                Vector<Frame> frames;
                {
                    EditorShell shell(layout);
                    scenario(shell, frames);
                }

                String suffix = kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png";
                for (int i = 0; i < frames.Count() && i < names.Count(); i++)
                    frames[i].bitmap->Save(outDir + names[i] + suffix, Bitmap::ImageType::Png);
            }
        }
    };

    void RaiseDialogs()
    {
        PushEditorScopeOnStack scope;

        auto root = EditorUIRoot.GetRootWidget();
        for (auto& child : Vector<Ref<Widget>>(root->GetChildWidgets()))
        {
            if (DynamicCast<o2::Window>(child) && !DynamicCast<DockableWindow>(child) && child->IsEnabled())
                child->SetIndexInSiblings(root->GetChildren().Count() - 1);
        }
    }

    void HideDialogs()
    {
        PushEditorScopeOnStack scope;

        for (auto& child : EditorUIRoot.GetRootWidget()->GetChildWidgets())
        {
            if (DynamicCast<o2::Window>(child) && !DynamicCast<DockableWindow>(child))
                child->Hide(true);
        }

        o2UI.FocusWidget(nullptr);
    }
}

TEST_F(EditorThemeGallery, EditorFrames)
{
    TinyScene scene;
    struct Case { String name; WindowsLayout layout; bool select; };
    Vector<Case> cases = {
        { "main", MakeLayout("scene window", "log window", "properties window"), true },
        { "assets", MakeLayout("scene window", "assets window", "properties window"), true },
        { "tabs", MakeLayout("pipeline window", "animation window", "game window"), false },
        { "graph", MakeLayout("animation state graph window", "log window", "properties window"), true },
    };

    for (auto& c : cases)
    {
        Shoot(c.layout, [&](EditorShell& shell, Vector<Frame>& frames)
        {
            o2Debug.Log("Scene loaded: Main.scn");
            o2Debug.LogWarning("Texture 'logo.png' has no mip maps");
            o2Debug.LogError("Script 'Player.js': ReferenceError: speed is not defined");

            if (c.select)
            {
                o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ scene.root->GetID() });
                o2EditorTree.GetSceneTree()->ExpandAll();
                o2EditorTree.GetSceneTree()->Focus();
            }
            frames.Add(shell.Settle());
        }, { c.name });
    }
}

TEST_F(EditorThemeGallery, DialogsAndMenus)
{
    auto curve = mmake<Curve>(Vector<Vec2F>({ Vec2F(0.0f, 0.0f), Vec2F(0.3f, 0.8f), Vec2F(0.7f, 0.2f), Vec2F(1.0f, 1.0f) }));

    Shoot(MakeLayout("scene window", "assets window", "properties window"), [&](EditorShell& shell, Vector<Frame>& frames)
    {
        ColorPickerDlg::Show(Color4(200, 120, 40, 200), [](const Color4&, bool) {});
        RaiseDialogs();
        frames.Add(shell.Settle());
        HideDialogs();

        CurveEditorDlg::RemoveAllEditingCurves();
        CurveEditorDlg::AddEditingCurve("curve", curve, Color4::Green());
        CurveEditorDlg::Show([]() {});
        RaiseDialogs();
        frames.Add(shell.Settle());
        CurveEditorDlg::RemoveAllEditingCurves();
        HideDialogs();

        YesNoCancelDlg::ShowYesNoCancel("Save the changes?", []() {});
        RaiseDialogs();
        frames.Add(shell.Settle());
        HideDialogs();

        PushEditorScopeOnStack scope;
        auto menu = o2UI.CreateWidget<ContextMenu>();
        menu->AddItem("Create/Empty actor");
        menu->AddItem("Create/Image", Function<void()>(), AssetRef<Asset>(o2Assets.GetAssetRef("ui/UI4_image_icn.svg")));
        menu->AddItem("Rename", Function<void()>(), AssetRef<Asset>(), ShortcutKeys({ VK_F2 }));
        menu->AddItem("---");
        menu->AddToggleItem("Locked", true);
        menu->AddToggleItem("Visible", false);
        menu->AddItem("Delete", Function<void()>(), AssetRef<Asset>(o2Assets.GetAssetRef("ui/UI4_remove_asset_instance.svg")));
        EditorUIRoot.AddWidget(menu);
        menu->Show(Vec2F(-200.0f, 150.0f));
        frames.Add(shell.Settle());
        menu->Hide(true);
        EditorUIRoot.RemoveWidget(menu);
    }, { "dialog_color", "dialog_curve", "dialog_yes_no", "context_menu" });
}

TEST_F(EditorThemeGallery, Preferences)
{
    Shoot(MakeLayout("scene window", "assets window", "properties window"), [&](EditorShell& shell, Vector<Frame>& frames)
    {
        static EditorPreferences preferences;
        preferences.theme = EditorTheme::GetKind();
        o2EditorPropertiesWindow.SetTarget(&preferences);
        frames.Add(shell.Settle());

        YesNoCancelDlg::ShowYesNo("Switch to the " +
                                  EditorTheme::NameOf(EditorTheme::IsDark() ? EditorThemeKind::Light : EditorThemeKind::Dark) +
                                  " theme after a restart?", []() {});
        RaiseDialogs();
        frames.Add(shell.Settle());
        HideDialogs();
    }, { "preferences", "restart_question" });
}

TEST_F(EditorThemeGallery, WidgetGallery)
{
    LightAfter lightAfter;
    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        BuildStyle(false);
        EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

        auto items = CollectGalleryItems();
        int pageCapacity = (frameSize.x/galleryCell.x)*(frameSize.y/galleryCell.y);
        int pages = (items.Count() + pageCapacity - 1)/pageCapacity;
        int perPage = (items.Count() + pages - 1)/Math::Max(pages, 1);
        int page = 1;
        Color4 background = kind == EditorThemeKind::Dark ? EditorTheme::StyleColor(Color4(251, 251, 251)) :
            Color4(226, 232, 232, 255);
        for (int begin = 0; begin < items.Count(); begin += perPage, page++)
        {
            Frame frame = RenderGalleryPage(items, begin, Math::Min(begin + perPage, items.Count()), background);
            frame.bitmap->Save(outDir + String::Format("gallery_%02i", page) +
                               (kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png"), Bitmap::ImageType::Png);
        }
    }
}

// Every image of the editor art on the panel of its theme, with its file name: for reviewing the restyled art
TEST_F(EditorThemeGallery, ArtSheet)
{
    LightAfter lightAfter;

    Vector<String> images;
    for (auto& file : o2FileSystem.GetFolderInfo(String(GetEditorAssetsPath()) + "ui").files)
    {
        String name = o2FileSystem.GetPathWithoutDirectories(file.path);
        if (name.EndsWith(".svg"))
            images.Add("ui/" + name);
    }
    images.Sort([](const String& a, const String& b) { return a < b; });

    const Vec2I cell(112, 64);
    const int columns = frameSize.x/cell.x, rows = frameSize.y/cell.y, perPage = columns*rows;

    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        Color4 background = kind == EditorThemeKind::Dark ? EditorTheme::StyleColor(Color4(251, 251, 251)) :
            Color4(251, 251, 251, 255);
        Color4 caption = EditorTheme::Get().text;

        for (int begin = 0, page = 1; begin < images.Count(); begin += perPage, page++)
        {
            Vector<Ref<VectorSprite>> sprites;
            Vector<Ref<Text>> captions;
            for (int i = begin; i < Math::Min(begin + perPage, images.Count()); i++)
            {
                int column = (i - begin)%columns, row = (i - begin)/columns;
                Vec2F cellOrigin(-frameSize.x*0.5f + column*cell.x, frameSize.y*0.5f - (row + 1)*cell.y);

                auto sprite = mmake<VectorSprite>(images[i]);
                Vec2F size = sprite->GetOriginalSize();
                float scale = Math::Min(1.0f, Math::Min((cell.x - 8)/Math::Max(size.x, 1.0f), (cell.y - 20)/Math::Max(size.y, 1.0f)));
                size *= scale;
                sprite->SetMode(SpriteMode::Default);
                sprite->SetRect(RectF(cellOrigin + Vec2F((cell.x - size.x)*0.5f, 16.0f), cellOrigin + Vec2F((cell.x + size.x)*0.5f, 16.0f + size.y)));
                sprites.Add(sprite);

                auto text = mmake<Text>("stdFont.ttf");
                text->text = o2FileSystem.GetFileNameWithoutExtension(images[i]).SubStr(4);
                text->height = 9;
                text->color = caption;
                text->horAlign = HorAlign::Middle;
                text->dotsEngings = true;
                text->SetRect(RectF(cellOrigin + Vec2F(2, 2), cellOrigin + Vec2F(cell.x - 2, 14)));
                captions.Add(text);
            }

            Frame frame = Capture([&]() {
                o2Render.Clear(background);
                for (auto& sprite : sprites)
                    sprite->Draw();
                for (auto& text : captions)
                    text->Draw();
            });
            frame.bitmap->Save(outDir + String::Format("art_%02i", page) + (kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png"),
                               Bitmap::ImageType::Png);
        }
    }
}

// The add node menu of the Pipeline window as its editor fills it, hovered over a category with its sub menu open
TEST_F(EditorThemeGallery, PipelineMenu)
{
    Shoot(MakeLayout("pipeline window", "assets window", "properties window"), [&](EditorShell& shell, Vector<Frame>& frames)
    {
        PushEditorScopeOnStack scope;

        auto menu = o2UI.CreateWidget<ContextMenu>();
        EditorUIRoot.AddWidget(menu);
        PipelineControls::SkinMenu(menu);

        static const Vector<Pair<PipelineNodeCategory, String>> categories = {
            { PipelineNodeCategory::Source, "Source" }, { PipelineNodeCategory::Transform, "Transform" },
            { PipelineNodeCategory::AI, "AI" }, { PipelineNodeCategory::Output, "Output" } };

        for (auto& category : categories)
        {
            for (auto schema : PipelineNodeRegistry::AllSchemas())
            {
                if (schema->category == category.first)
                {
                    menu->AddItem("Add " + category.second + "/" + schema->label, []() {},
                                  o2Assets.GetAssetRef(PipelineNodeWidget::MenuIconForType(schema->type)));
                }
            }
        }
        menu->AddItem("---");
        menu->AddItem("Paste", []() {}, AssetRef<Asset>(), ShortcutKeys({ VK_V, VK_CTRL_CMD }));
        menu->AddItem("Fit view", []() {});

        menu->Show(Vec2F(-300.0f, 200.0f));
        shell.Settle();

        // The cursor over the first category: it is hovered and opens its sub menu. The shell runs no input events, so
        // the menu is told where the cursor is
        struct HoverAccess : public ContextMenu { using ContextMenu::UpdateHover; };
        Ref<ContextMenuItem> category;
        for (auto& child : menu->GetItemsLayout()->GetChildWidgets())
        {
            auto item = DynamicCast<ContextMenuItem>(child);
            if (item && item->GetSubMenu() && (!category || item->layout->GetWorldRect().top > category->layout->GetWorldRect().top))
                category = item;
        }

        if (category)
        {
            Vec2F point = category->layout->GetWorldRect().Center();
            o2Input.OnCursorMoved(point, 0, false);
            for (int i = 0; i < 60; i++)
            {
                static_cast<HoverAccess*>(menu.Get())->UpdateHover(point);
                shell.Update(1.0f/60.0f);
            }

            category->GetSubMenu()->Show(menu, category->layout->worldRightTop);
            for (int i = 0; i < 30; i++)
                shell.Update(1.0f/60.0f);
        }

        frames.Add(Capture([&]() { shell.Draw(); }));
        menu->Hide(true);
        EditorUIRoot.RemoveWidget(menu);
    }, { "pipeline_menu" });
}

// The curve and the spline editors with every point selected: their curves, guides to the handles and the selection frames
TEST_F(EditorThemeGallery, FramesAndGuides)
{
    LightAfter lightAfter;
    for (auto kind : { EditorThemeKind::Light, EditorThemeKind::Dark })
    {
        EditorTheme::SetKind(kind);
        BuildStyle(false);
        EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
        PushEditorScopeOnStack scope;

        auto page = mmake<Widget>();
        *page->layout = WidgetLayout::BothStretch();
        EditorUIRoot.AddWidget(page);

        auto curve = mmake<Curve>(Vector<Vec2F>({ Vec2F(0.0f, 0.0f), Vec2F(0.3f, 0.8f), Vec2F(0.7f, 0.2f), Vec2F(1.0f, 1.0f) }));
        auto curves = mmake<CurvesEditor>();
        *curves->layout = WidgetLayout(Vec2F(0.0f, 0.0f), Vec2F(0.5f, 1.0f), Vec2F(10.0f, 10.0f), Vec2F(-10.0f, -10.0f));
        curves->SetMainHandleImages(AssetRef<VectorImageAsset>("ui/CurveHandle.svg"), AssetRef<VectorImageAsset>("ui/CurveHandleHover.svg"),
                                    AssetRef<VectorImageAsset>("ui/CurveHandlePressed.svg"), AssetRef<VectorImageAsset>("ui/CurveHandleSelected.svg"));
        curves->SetSupportHandleImages(AssetRef<VectorImageAsset>("ui/CurveSupportHandle.svg"),
                                       AssetRef<VectorImageAsset>("ui/CurveSupportHandleHover.svg"),
                                       AssetRef<VectorImageAsset>("ui/CurveSupportHandlePressed.svg"),
                                       AssetRef<VectorImageAsset>("ui/CurveSupportHandleSelected.svg"));
        page->AddChild(curves);
        curves->AddCurve("curve", curve);

        auto spline = mmake<Spline>();
        spline->BeginKeysBatchChange();
        spline->AppendKey(Vec2F(140.0f, -200.0f), 0.0f, Vec2F(-60.0f, -40.0f), Vec2F(60.0f, 40.0f));
        spline->AppendKey(Vec2F(560.0f, -120.0f), 0.0f, Vec2F(-40.0f, -80.0f), Vec2F(40.0f, 80.0f));
        spline->AppendKey(Vec2F(480.0f, 220.0f), 0.0f, Vec2F(80.0f, -20.0f), Vec2F(-80.0f, 20.0f));
        spline->AppendKey(Vec2F(180.0f, 160.0f), 0.0f, Vec2F(40.0f, 60.0f), Vec2F(-40.0f, -60.0f));
        spline->CompleteKeysBatchingChange();
        spline->SetClosed(true);

        auto wrapper = mmake<ScreenSplineWrapper>();
        wrapper->spline = spline;
        auto splineEditor = mmake<SplineEditor>();
        splineEditor->SetSpline(wrapper);

        page->SetEnabledForcible(false);
        page->SetEnabledForcible(true);
        ParkCursor();
        for (int i = 0; i < settleSteps; i++)
            UpdateRoot(settleStep);

        curves->SelectAll();
        splineEditor->SelectAll();
        for (int i = 0; i < settleSteps/2; i++)
        {
            UpdateRoot(settleStep);
            splineEditor->Update(settleStep);
        }

        // The handles fade their looks in by the frame time
        Color4 background = EditorTheme::Get().viewBack;
        for (int i = 0; i < 20; i++)
        {
            o2Time.Update(0.05f);
            o2Render.Begin();
            o2Render.Clear(background);
            page->Draw();
            splineEditor->Draw();
            o2Render.End();
        }

        Frame frame = Capture([&]() {
            o2Render.Clear(background);
            page->Draw();
            splineEditor->Draw();
        });
        frame.bitmap->Save(outDir + "frames_guides" + (kind == EditorThemeKind::Dark ? "_dark.png" : "_light.png"),
                           Bitmap::ImageType::Png);

        splineEditor->Reset();
        EditorUIRoot.RemoveWidget(page);
    }
}
