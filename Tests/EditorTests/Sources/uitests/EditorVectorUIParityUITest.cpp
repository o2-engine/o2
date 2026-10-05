#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <set>

#include "o2/Utils/Bitmap/BitmapCompare.h"
#include "EditorVectorUIFixture.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

// The editor UI drawn the old way, with raster sprites, against the same UI drawn with vector graphics

namespace
{
    const String shotsFolder = "TestScreenshots/vector_parity/";

    const float minSimilarity = 0.99f;
    const int assertTolerance = 4;
    const int reportTolerances[] = { 0, 2, 4, 8, 16, 24 };

    int CountDistinctColors(const Bitmap& bitmap)
    {
        std::set<UInt32> colors;
        const UInt32* data = (const UInt32*)bitmap.GetData();
        size_t pixels = (size_t)bitmap.GetSize().x*bitmap.GetSize().y;
        for (size_t i = 0; i < pixels; i++)
            colors.insert(data[i]);

        return (int)colors.size();
    }

    bool AreSame(const Bitmap& a, const Bitmap& b)
    {
        return a.GetSize() == b.GetSize() &&
            memcmp(a.GetData(), b.GetData(), (size_t)a.GetSize().x*a.GetSize().y*4) == 0;
    }

    // Compares the passes of a scene, saves raster, vector and difference images and prints the similarity table
    void ExpectParity(const String& name, const Frame& raster, const Frame& vector, int minVectorTriangles)
    {
        ASSERT_TRUE(raster.bitmap && vector.bitmap) << name.Data();
        ASSERT_EQ(raster.bitmap->GetSize(), frameSize) << name.Data();
        ASSERT_EQ(vector.bitmap->GetSize(), frameSize) << name.Data();

        o2FileSystem.FolderCreate(shotsFolder, true);
        raster.bitmap->Save(shotsFolder + name + "_raster.png", Bitmap::ImageType::Png);
        vector.bitmap->Save(shotsFolder + name + "_vector.png", Bitmap::ImageType::Png);

        EXPECT_LE(raster.vectorTriangles*30, vector.vectorTriangles)
            << name.Data() << ": the raster pass has drawn " << raster.vectorTriangles << " vector triangles";
        EXPECT_GT(vector.vectorTriangles, (UInt64)minVectorTriangles) << name.Data() << ": the vector pass has drawn too few";
        EXPECT_GT(CountDistinctColors(*raster.bitmap), 64) << name.Data() << ": the raster frame is blank";
        EXPECT_GT(CountDistinctColors(*vector.bitmap), 64) << name.Data() << ": the vector frame is blank";
        EXPECT_FALSE(AreSame(*raster.bitmap, *vector.bitmap)) << name.Data() << ": the passes are byte-identical";

        Bitmap difference;
        BitmapCompareResult asserted = BitmapCompare::Compare(*raster.bitmap, *vector.bitmap, assertTolerance,
                                                              Color4::White(), &difference);
        ASSERT_TRUE(asserted.comparable) << name.Data();
        difference.Save(shotsFolder + name + "_diff.png", Bitmap::ImageType::Png);

        String table;
        for (int tolerance : reportTolerances)
        {
            float similarity = BitmapCompare::GetSimilarity(*raster.bitmap, *vector.bitmap, tolerance);
            table += String::Format(" tol%i %.4f", tolerance, similarity);
        }

        int pixels = frameSize.x*frameSize.y;
        printf("  %s:%s; mean %.3f, max %i, beyond tolerance %i: %i px; vector triangles %llu, in the raster pass %llu\n", name.Data(), table.Data(),
               asserted.meanDifference, asserted.maxDifference, assertTolerance,
               (int)Math::Round((1.0f - asserted.similarity)*(float)pixels), (unsigned long long)vector.vectorTriangles, (unsigned long long)raster.vectorTriangles);

        EXPECT_GE(asserted.similarity, minSimilarity) << name.Data() << " at tolerance " << assertTolerance;
    }

    struct GalleryItem
    {
        Ref<Widget> sample;
        String      state;         // Forced state, empty for the default look
        bool        value = false; // Value of the forced state
    };

    const Vec2I galleryCell(170, 76);
    const float galleryCellMargin = 6.0f;
    const Color4 galleryBackground(226, 232, 232, 255);

    // Every style in the default look and once more for each of its states turned over.
    // AnimationTree is left out: it draws through its animation window and has none outside of it
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

    String DescribeGalleryItems(const Vector<GalleryItem>& items)
    {
        String res;
        for (auto& item : items)
            res += item.sample->GetType().GetName() + ":" + item.sample->GetName() + ":" + item.state + "\n";

        return res;
    }

    Frame RenderGalleryPage(const Vector<GalleryItem>& items, int begin, int end)
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
            o2Render.Clear(galleryBackground);
            page->Draw();
        });

        EditorUIRoot.RemoveWidget(page);
        return res;
    }

    using EditorVectorUIParity = EditorVectorUIFixture;
}

TEST_F(EditorVectorUIParity, EditorFrame)
{
    TinyScene scene;
    Vector<String> names = { "editor_frame", "editor_frame_tabs", "editor_frame_selection" };
    Vector<WindowsLayout> layouts = { MakeLayout("scene window", "assets window", "properties window"),
                                      MakeLayout("pipeline window", "animation window", "game window"),
                                      MakeLayout("animation state graph window", "log window", "properties window") };

    for (int i = 0; i < names.Count(); i++)
    {
        auto scenario = [&](EditorShell& shell, Vector<Frame>& frames)
        {
            if (i == 2)
            {
                o2Debug.Log("Vector parity: regular message");
                o2Debug.LogWarning("Vector parity: warning message");
                o2Debug.LogError("Vector parity: error message");
                o2EditorSceneScreen.SelectObjectsByIdsWithoutAction({ scene.root->GetID() });
                o2EditorTree.GetSceneTree()->ExpandAll();
            }

            frames.Add(shell.Settle());
        };

        auto raster = RenderEditor(true, scenario, layouts[i]);
        auto vector = RenderEditor(false, scenario, layouts[i]);

        ASSERT_EQ(raster.Count(), 1);
        ASSERT_EQ(vector.Count(), 1);
        ExpectParity(names[i], raster[0], vector[0], 20000);
    }
}

TEST_F(EditorVectorUIParity, DialogsOverEditor)
{
    Vector<String> names = { "dialog_color_picker", "dialog_curve_editor", "dialog_name_edit", "dialog_yes_no_cancel",
                             "context_menu" };

    auto curve = mmake<Curve>(Vector<Vec2F>({ Vec2F(0.0f, 0.0f), Vec2F(0.3f, 0.8f), Vec2F(0.7f, 0.2f), Vec2F(1.0f, 1.0f) }));

    auto scenario = [&](EditorShell& shell, Vector<Frame>& frames)
    {
        frames.Add(shell.Settle());

        ColorPickerDlg::Show(Color4(200, 120, 40, 200), [](const Color4&, bool) {});
        shell.RaiseDialogs();
        frames.Add(shell.Settle());
        shell.HideDialogs();

        CurveEditorDlg::RemoveAllEditingCurves();
        CurveEditorDlg::AddEditingCurve("curve", curve, Color4::Green());
        CurveEditorDlg::Show([]() {});
        shell.RaiseDialogs();
        frames.Add(shell.Settle());
        CurveEditorDlg::RemoveAllEditingCurves();
        shell.HideDialogs();

        NameEditDlg::Show("Some name", [](const String&) {});
        shell.RaiseDialogs();
        frames.Add(shell.Settle());
        shell.HideDialogs();

        YesNoCancelDlg::ShowYesNoCancel("Save the changes?", []() {});
        shell.RaiseDialogs();
        frames.Add(shell.Settle());
        shell.HideDialogs();

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
    };

    ASSERT_TRUE(o2Assets.IsAssetExist("ui/UI4_image_icn.svg"));

    auto raster = RenderEditor(true, scenario);
    auto vector = RenderEditor(false, scenario);

    ASSERT_EQ(raster.Count(), names.Count() + 1);
    ASSERT_EQ(vector.Count(), names.Count() + 1);
    for (int i = 0; i < names.Count(); i++)
    {
        ExpectParity(names[i], raster[i + 1], vector[i + 1], 20000);

        EXPECT_LT(BitmapCompare::GetSimilarity(*vector[0].bitmap, *vector[i + 1].bitmap, 4), 0.99f)
            << names[i].Data() << " is not seen over the editor";
    }
}

TEST_F(EditorVectorUIParity, WidgetGallery)
{
    Vector<Frame> frames[2];
    String descriptions[2];
    int itemsCount = 0;
    int pageCapacity = (frameSize.x/galleryCell.x)*(frameSize.y/galleryCell.y);

    for (int pass = 0; pass < 2; pass++)
    {
        BuildStyle(pass == 0);

        // Some styles reach the editor windows when they are updated
        EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

        auto items = CollectGalleryItems();
        descriptions[pass] = DescribeGalleryItems(items);
        itemsCount = items.Count();

        // Spread evenly, a last page of a few widgets would be an almost blank frame
        int pages = (items.Count() + pageCapacity - 1)/pageCapacity;
        int perPage = (items.Count() + pages - 1)/Math::Max(pages, 1);
        for (int begin = 0; begin < items.Count(); begin += perPage)
            frames[pass].Add(RenderGalleryPage(items, begin, Math::Min(begin + perPage, items.Count())));
    }

    ASSERT_GT(itemsCount, 300) << "the style has too few widgets";
    ASSERT_EQ(descriptions[0], descriptions[1]) << "the passes have built different styles";
    ASSERT_EQ(frames[0].Count(), frames[1].Count());

    o2FileSystem.FolderCreate(shotsFolder, true);
    o2FileSystem.WriteFile(shotsFolder + "gallery_items.txt", descriptions[1]);

    printf("  %i gallery items of %i styles on %i pages\n", itemsCount, (int)o2UI.GetWidgetStyles().Count(),
           (int)frames[0].Count());

    for (int i = 0; i < frames[0].Count(); i++)
        ExpectParity(String::Format("gallery_%02i", i + 1), frames[0][i], frames[1][i], 100);
}
