#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/EditorConfig.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Sound/SoundPlayer.h"

using namespace o2;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    void DrawOnce(const Ref<Widget>& widget)
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

    // Lays the icon of the grid out with the name and returns the lines of its label
    Vector<Text::SymbolsSet::Line> LayoutIconName(const Ref<AssetIcon>& icon, const WString& name, RectF& labelRect)
    {
        Vec2F cell = icon->layout->GetMinimalSize();
        *icon->layout = WidgetLayout::Based(BaseCorner::Center, cell);
        icon->layout->minSize = cell;
        icon->SetAssetName(name);
        DrawOnce(icon);
        DrawOnce(icon);

        auto label = icon->GetChildByType<Label>("nameLabel");
        labelRect = label->layout->GetWorldRect();
        return label->GetLayerDrawable<Text>("text")->GetSymbolsSet().mLines;
    }
}

namespace
{
// Updates and draws the editor as an application frame does, then lets the input go
void ShellStep(EditorShell& shell, int frames = 1)
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

void DoubleClick(EditorShell& shell, const Vec2F& point)
{
    // A press soon after the previous one is a double click: the first one waits for the previous to pass
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    o2Input.OnCursorMoved(point, 0, false);
    ShellStep(shell, 2);
    o2Input.OnCursorPressed(point);
    ShellStep(shell);
    o2Input.OnCursorReleased();
    ShellStep(shell);
    o2Input.OnCursorPressed(point);
    ShellStep(shell);
    o2Input.OnCursorReleased();
    ShellStep(shell, 3);
}

bool IsFolder(const Ref<AssetInfo>& info)
{
    return info->meta->GetAssetType() == &TypeOf(FolderAsset);
}

Ref<AssetInfo> FindFolderWithFiles(const Vector<Ref<AssetInfo>>& infos, int count)
{
    for (auto& info : infos)
    {
        if (!IsFolder(info))
            continue;

        int files = 0;
        for (auto& child : info->GetChildren())
            files += IsFolder(child) ? 0 : 1;

        if (files >= count)
            return info;

        if (auto found = FindFolderWithFiles(info->GetChildren(), count))
            return found;
    }

    return nullptr;
}

Ref<AssetInfo> FindFolderWithSubfolder(const Vector<Ref<AssetInfo>>& infos)
{
    for (auto& info : infos)
    {
        if (!IsFolder(info))
            continue;

        for (auto& child : info->GetChildren())
        {
            if (IsFolder(child))
                return info;
        }

        if (auto found = FindFolderWithSubfolder(info->GetChildren()))
            return found;
    }

    return nullptr;
}

// Stops the renaming of the icon without renaming the asset
void CancelRenaming(const Ref<AssetIcon>& icon)
{
    if (auto editBox = icon->GetChildByType<EditBox>("nameEditBox"))
        editBox->onChangeCompleted = Function<void(const WString&)>();

    icon->SetState("edit", false);
}

Int64 FileSize(const Ref<AssetInfo>& info)
{
    return o2FileSystem.GetFileInfo(o2Assets.GetAssetsPath() + info->path).size;
}

String ViewIcon(const Ref<Button>& button)
{
    auto icon = DynamicCast<VectorSprite>(button->GetIconDrawable());
    return icon ? icon->GetImageName() : String();
}

// Returns the color of the captured frame at the world point of the default camera
Color4 FramePixel(const Ref<Bitmap>& frame, const Vec2F& point)
{
    Vec2I size = frame->GetSize();
    float scale = (float)size.x/(float)o2Render.GetResolution().x;
    Vec2F half = Vec2F(o2Render.GetResolution())*0.5f;
    int x = Math::Clamp((int)((point.x + half.x)*scale), 0, size.x - 1);
    int y = Math::Clamp((int)((point.y + half.y)*scale), 0, size.y - 1);
    const UInt8* pixel = frame->GetData() + ((size_t)y*size.x + x)*4;
    return Color4((int)pixel[0], (int)pixel[1], (int)pixel[2], (int)pixel[3]);
}

int ColorDiff(const Color4& a, const Color4& b)
{
    return Math::Abs(a.r - b.r) + Math::Abs(a.g - b.g) + Math::Abs(a.b - b.b);
}

// The glyph bitmaps have a border of two pixels around the ink
RectF GlyphInk(const RectF& frame)
{
    float border = 2.0f/o2Render.GetTargetPixelDensity();
    return RectF(frame.left + border, frame.top - border, frame.right - border, frame.bottom + border);
}
}

class AssetsWindowViews: public EditorVectorUIFixture
{};

// A name of three lines fits the cell of the grid whole, a fourth line stays out of it
TEST_F(AssetsWindowViews, IconNameOfThreeLinesFitsTheCell)
{
    BuildStyle(false);
    PushEditorScopeOnStack scope;

    auto icon = o2UI.CreateWidget<AssetIcon>();

    RectF labelRect;
    auto lines = LayoutIconName(icon, "tile_medium_cream.svg", labelRect);
    ASSERT_EQ(lines.Count(), 3);

    float inkTop = -FLT_MAX, inkBottom = FLT_MAX;
    for (auto& line : lines)
    {
        for (auto& symbol : line.mSymbols)
        {
            RectF ink = GlyphInk(symbol.mFrame);
            if (ink.Width() < 0.01f)
                continue;

            inkTop = Math::Max(inkTop, ink.top);
            inkBottom = Math::Min(inkBottom, ink.bottom);
        }
    }

    EXPECT_LE(inkTop, labelRect.top + 0.01f) << "the name is cut at the top";
    EXPECT_GE(inkBottom, labelRect.bottom - 0.01f) << "the third line is cut";
    EXPECT_LE(labelRect.Height() - (inkTop - inkBottom), 1.0f) << "the label is higher than the three lines";

    // The icons of the types are transparent at the bottom fifth of their height
    RectF iconRect = icon->layer["icon"]->GetRect();
    RectF cellRect = icon->layout->GetWorldRect();
    EXPECT_GE(iconRect.Width(), 48.0f) << "the icons of the types are small";
    EXPECT_GE(iconRect.bottom + iconRect.Height()*0.2f, labelRect.top - 0.01f) << "the icon covers the name";
    EXPECT_GE(iconRect.left, cellRect.left - 0.01f);
    EXPECT_LE(iconRect.right, cellRect.right + 0.01f);

    lines = LayoutIconName(icon, "a_name_of_an_asset_of_four_lines.svg", labelRect);
    ASSERT_GE(lines.Count(), 4);
    for (auto& symbol : lines[3].mSymbols)
        EXPECT_LE(GlyphInk(symbol.mFrame).top, labelRect.bottom + 0.01f) << "a fourth line shows in the cell";
}

// The list and the table show rows of the view width; the table has the columns under a header that sorts them, with
// light lines between the columns and no back; one button turns the views in a circle
TEST_F(AssetsWindowViews, ListAndTableShowRowsAndTheHeaderSortsThem)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto folder = FindFolderWithFiles(o2Assets.GetAssetsTree().rootAssets, 3);
    ASSERT_TRUE(folder) << "no folder with three files in the assets";

    auto& assets = o2EditorAssets;
    assets.OpenFolder(folder->path);
    shell.Settle();

    auto scroll = assets.GetAssetsScroll();
    auto viewButton = assets.GetViewButton();
    ASSERT_TRUE(viewButton);
    EXPECT_EQ(ViewIcon(viewButton), String("ui/UI4_grid_view_icon.svg"));

    RectF viewRect = viewButton->layout->GetWorldRect(), filterRect = assets.GetFilterButton()->layout->GetWorldRect();
    EXPECT_NEAR(viewRect.right, filterRect.left + 1.0f, 0.01f) << "the edge lines of the buttons are not one";
    EXPECT_NEAR(viewRect.Height(), filterRect.Height(), 0.01f);

    // The edge lines of the button take its top 19 points: the icon is in the middle of them
    RectF viewIconRect = viewButton->GetLayer("icon")->GetRect();
    EXPECT_NEAR(viewIconRect.Center().x, viewRect.Center().x, 0.01f);
    EXPECT_NEAR(viewIconRect.Center().y, viewRect.Center().y + 0.5f, 0.01f);

    viewButton->onClick();
    shell.Settle();
    EXPECT_EQ(assets.GetViewMode(), 1);
    EXPECT_EQ(ViewIcon(viewButton), String("ui/UI4_list_icon.svg"));
    EXPECT_EQ(o2EditorConfig.projectConfig.assetsView, String("list"));

    RectF scrollRect = scroll->layout->GetWorldRect();
    auto rows = scroll->GetVisibleIcons();
    ASSERT_GE(rows.Count(), 3);
    for (int i = 0; i < rows.Count(); i++)
    {
        RectF rect = rows[i]->layout->GetWorldRect();
        EXPECT_NEAR(rect.Width(), scrollRect.Width() - 10.0f, 1.0f);
        EXPECT_NEAR(rect.Height(), 20.0f, 0.01f);
        if (i > 0)
            EXPECT_NEAR(rect.top, rows[i - 1]->layout->GetWorldRect().bottom, 0.01f);

        EXPECT_TRUE(rows[i]->IsEnabledInHierarchy()) << "a row made after the view was shown is not drawn";
    }

    Ref<AssetInfo> selected = scroll->GetAssetInfos()[1];
    scroll->SelectAsset(selected->meta->ID(), false);

    viewButton->onClick();
    auto frame = shell.Settle();
    EXPECT_EQ(o2EditorConfig.projectConfig.assetsView, String("table"));
    EXPECT_EQ(ViewIcon(viewButton), String("ui/UI4_table_view_icon.svg"));

    auto header = scroll->GetTableHeader();
    ASSERT_TRUE(header);
    EXPECT_TRUE(header->IsEnabledInHierarchy());
    auto columns = header->GetChildWidgets();
    ASSERT_EQ(columns.Count(), 4);

    rows = scroll->GetVisibleIcons();
    ASSERT_GE(rows.Count(), 3);
    for (auto& row : rows)
        EXPECT_EQ(row->IsSelected(), row->GetAssetInfo() == selected) << row->GetAssetInfo()->path;

    const char* labels[] = { "nameLabel", "typeLabel", "sizeLabel", "dateLabel" };
    for (int column = 1; column < 4; column++)
    {
        RectF cell = rows[0]->GetChildWidget(labels[column])->layout->GetWorldRect();
        RectF title = columns[column]->layout->GetWorldRect();
        EXPECT_NEAR(cell.left, title.left, 0.5f) << labels[column];
        EXPECT_NEAR(cell.right, title.right, 0.5f) << labels[column];
    }

    // A line between the name and the type, the header has the back of the rows
    Ref<AssetIcon> plainRow = rows[0]->IsSelected() ? rows[1] : rows[0];
    float lineX = Math::Round(columns[1]->layout->GetWorldRect().left - 4.0f);
    float rowY = plainRow->layout->GetWorldRect().Center().y;
    Color4 nearLine = FramePixel(frame.bitmap, Vec2F(lineX + 3.5f, rowY));
    int lineDiff = Math::Max(ColorDiff(FramePixel(frame.bitmap, Vec2F(lineX - 0.5f, rowY)), nearLine),
                             ColorDiff(FramePixel(frame.bitmap, Vec2F(lineX + 0.5f, rowY)), nearLine));
    EXPECT_GT(lineDiff, 10) << "no line between the columns";

    RectF typeTitle = columns[1]->layout->GetWorldRect();
    Vec2F headerEmpty(typeTitle.right - 20.0f, typeTitle.Center().y);
    Vec2F rowEmpty(typeTitle.right - 20.0f, plainRow->layout->GetWorldRect().top - 2.0f);
    EXPECT_LT(ColorDiff(FramePixel(frame.bitmap, headerEmpty), FramePixel(frame.bitmap, rowEmpty)), 8) << "the header has a back";

    for (auto& row : rows)
    {
        auto info = row->GetAssetInfo();
        if (IsFolder(info))
            continue;

        auto file = o2FileSystem.GetFileInfo(o2Assets.GetAssetsPath() + info->path);
        EXPECT_EQ((String)row->GetChildByType<Label>("typeLabel")->GetText(), (String)AssetsIconsScrollArea::GetTypeText(*info));
        EXPECT_EQ((String)row->GetChildByType<Label>("sizeLabel")->GetText(), (String)AssetsIconsScrollArea::GetSizeText(*info, file));
        EXPECT_EQ(row->GetChildByType<Label>("dateLabel")->GetText().Length(), 16) << info->path;
    }

    auto expectSizesSorted = [&](bool descending) {
        auto& infos = scroll->GetAssetInfos();
        bool filesStarted = false;
        for (int i = 0; i < infos.Count(); i++)
        {
            if (IsFolder(infos[i]))
            {
                EXPECT_FALSE(filesStarted) << "a folder after the files: " << infos[i]->path;
                continue;
            }

            if (filesStarted && !IsFolder(infos[i - 1]))
            {
                if (descending)
                    EXPECT_GE(FileSize(infos[i - 1]), FileSize(infos[i])) << infos[i]->path;
                else
                    EXPECT_LE(FileSize(infos[i - 1]), FileSize(infos[i])) << infos[i]->path;
            }

            filesStarted = true;
        }
    };

    auto sizeButton = DynamicCast<Button>(columns[2]);
    ASSERT_TRUE(sizeButton);
    auto sizeArrow = sizeButton->GetLayer("sortArrow");
    auto nameArrow = columns[0]->GetLayer("sortArrow");
    EXPECT_TRUE(nameArrow->IsEnabled());
    EXPECT_FALSE(sizeArrow->IsEnabled());

    sizeButton->onClick();
    EXPECT_EQ(scroll->GetSortBy(), AssetsIconsScrollArea::SortBy::Size);
    EXPECT_FALSE(scroll->IsSortDescending());
    EXPECT_TRUE(sizeArrow->IsEnabled());
    EXPECT_FALSE(nameArrow->IsEnabled());
    EXPECT_NEAR(sizeArrow->GetDrawable()->GetAngle(), Math::PI(), 0.01f) << "the arrow of the ascending sorting points up";
    expectSizesSorted(false);

    sizeButton->onClick();
    EXPECT_TRUE(scroll->IsSortDescending());
    EXPECT_NEAR(sizeArrow->GetDrawable()->GetAngle(), 0.0f, 0.01f);
    expectSizesSorted(true);

    viewButton->onClick();
    shell.Settle();
    EXPECT_EQ(assets.GetViewMode(), 0);
    EXPECT_EQ(ViewIcon(viewButton), String("ui/UI4_grid_view_icon.svg"));
    EXPECT_FALSE(header->IsEnabledInHierarchy());
    ASSERT_FALSE(scroll->GetVisibleIcons().IsEmpty());
    EXPECT_NEAR(scroll->GetVisibleIcons()[0]->layout->GetWorldRect().Width(), 50.0f, 0.01f);
}

// The hover slides to the icon under the cursor and fades in, as the one of the folders tree does, and fades out away
TEST_F(AssetsWindowViews, HoverSlidesToTheIconUnderTheCursor)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto folder = FindFolderWithFiles(o2Assets.GetAssetsTree().rootAssets, 3);
    ASSERT_TRUE(folder);

    auto& assets = o2EditorAssets;
    assets.OpenFolder(folder->path);
    shell.Settle();

    auto scroll = assets.GetAssetsScroll();
    auto hover = scroll->GetHoverDrawable();
    ASSERT_TRUE(hover);

    for (int mode : { 0, 1 })
    {
        assets.SetViewMode(mode);
        ParkCursor();
        ShellStep(shell, 30);
        EXPECT_LT(hover->GetTransparency(), 0.05f) << "mode " << mode;

        auto icons = scroll->GetVisibleIcons();
        ASSERT_GE(icons.Count(), 3);
        EXPECT_FALSE(icons[0]->state["hover"]) << "an icon has its own hover";

        Layout hoverLayout = Layout::BothStretch(-10, -16, -10, -16);
        o2Input.OnCursorMoved(icons[0]->layout->GetWorldRect().Center(), 0, false);
        ShellStep(shell, 40);
        EXPECT_EQ(scroll->GetHoveredIcon(), icons[0]);
        EXPECT_GT(hover->GetTransparency(), 0.95f);

        RectF first = hoverLayout.Calculate(icons[0]->layout->GetWorldRect());
        RectF third = hoverLayout.Calculate(icons[2]->layout->GetWorldRect());
        EXPECT_NEAR(hover->GetRect().left, first.left, 1.0f);
        EXPECT_NEAR(hover->GetRect().top, first.top, 1.0f);

        o2Input.OnCursorMoved(icons[2]->layout->GetWorldRect().Center(), 0, false);
        ShellStep(shell, 2);
        EXPECT_EQ(scroll->GetHoveredIcon(), icons[2]);
        float slid = (hover->GetRect().Center() - first.Center()).Length();
        float way = (third.Center() - first.Center()).Length();
        EXPECT_GT(slid, 1.0f) << "the hover stays";
        EXPECT_LT(slid, way - 1.0f) << "the hover jumps";

        ShellStep(shell, 60);
        EXPECT_NEAR(hover->GetRect().left, third.left, 1.0f);
        EXPECT_NEAR(hover->GetRect().top, third.top, 1.0f);
        EXPECT_GT(hover->GetTransparency(), 0.95f);

        ParkCursor();
        ShellStep(shell, 60);
        EXPECT_FALSE(scroll->GetHoveredIcon());
        EXPECT_LT(hover->GetTransparency(), 0.05f);
    }
}

// A double click on the name renames an asset only in the grid; the rows open it. The context menu renames in any view
TEST_F(AssetsWindowViews, RenamingIsByTheContextMenuInTheListAndTheTable)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto folder = FindFolderWithSubfolder(o2Assets.GetAssetsTree().rootAssets);
    ASSERT_TRUE(folder);

    auto& assets = o2EditorAssets;
    auto scroll = assets.GetAssetsScroll();
    auto findSubfolderIcon = [&]() {
        for (auto& icon : scroll->GetVisibleIcons())
        {
            if (IsFolder(icon->GetAssetInfo()))
                return icon;
        }

        return Ref<AssetIcon>();
    };

    assets.OpenFolder(folder->path);
    shell.Settle();

    auto icon = findSubfolderIcon();
    ASSERT_TRUE(icon);
    DoubleClick(shell, icon->GetChildWidget("nameLabel")->layout->GetWorldRect().Center());
    EXPECT_TRUE(icon->GetState("edit")) << "the grid renames by a double click on the name";
    EXPECT_EQ(assets.GetOpenedFolderPath(), folder->path);
    CancelRenaming(icon);

    assets.SetViewMode(1);
    shell.Settle();
    icon = findSubfolderIcon();
    ASSERT_TRUE(icon);
    String subfolder = icon->GetAssetInfo()->path;
    DoubleClick(shell, icon->GetChildWidget("nameLabel")->layout->GetWorldRect().Center());
    EXPECT_FALSE(icon->GetState("edit")) << "the list renames by a double click";
    EXPECT_EQ(assets.GetOpenedFolderPath(), subfolder) << "the list opens the folder by a double click";
    CancelRenaming(icon);

    Ref<ContextMenu::Item> rename;
    for (auto& item : scroll->GetContextMenu()->GetItems())
    {
        if (item->text == WString("Rename"))
            rename = item;
    }

    ASSERT_TRUE(rename) << "no Rename in the context menu";
    EXPECT_FALSE(rename->GetShortcut().IsEmpty());

    for (int mode : { 2, 0 })
    {
        assets.OpenFolder(folder->path);
        assets.SetViewMode(mode);
        shell.Settle();

        auto target = scroll->GetAssetInfos().Last();
        scroll->SelectAsset(target->meta->ID(), false);
        rename->onClick();
        ShellStep(shell, 2);

        Ref<AssetIcon> renamed;
        for (auto& visible : scroll->GetVisibleIcons())
        {
            if (visible->GetAssetInfo() == target)
                renamed = visible;
        }

        ASSERT_TRUE(renamed) << "mode " << mode;
        EXPECT_TRUE(renamed->GetState("edit")) << "mode " << mode;
        auto editBox = renamed->GetChildByType<EditBox>("nameEditBox");
        EXPECT_EQ((String)editBox->GetText(),
                  o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(target->path))) << "mode " << mode;
        CancelRenaming(renamed);
    }
}

// The editor starts in the view kept in the config; the icons of the grid made before it are released without a crash
TEST_F(AssetsWindowViews, StartsInTheViewOfTheConfig)
{
    BuildStyle(false);
    for (const char* view : { "table", "list" })
    {
        EditorShell shell(MakeLayout("scene window", "assets window", "properties window"),
                          [&](EditorConfig& config) { config.projectConfig.assetsView = view; });

        auto& assets = o2EditorAssets;
        EXPECT_EQ(assets.GetViewMode(), String(view) == "table" ? 2 : 1);
        shell.Settle();

        auto folder = FindFolderWithFiles(o2Assets.GetAssetsTree().rootAssets, 3);
        ASSERT_TRUE(folder);
        for (int i = 0; i < 3; i++)
        {
            assets.OpenFolder(folder->path);
            ShellStep(shell, 3);
            assets.OpenFolder("");
            ShellStep(shell, 3);
            assets.SetViewMode((assets.GetViewMode() + 1)%3);
            ShellStep(shell, 3);
        }

        EXPECT_FALSE(assets.GetAssetsScroll()->GetVisibleIcons().IsEmpty()) << view;
    }
}

// A sound shows the icon of the sounds, nothing plays on it; a video shows its frame with a play badge outlined light
TEST_F(AssetsWindowViews, SoundsShowTheirIconAndVideosTheirFrameWithABadge)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    const String folder = "Generated/GeminiShowcase";
    ASSERT_TRUE(o2Assets.IsAssetExist(folder + "/voice.wav"));
    ASSERT_TRUE(o2Assets.IsAssetExist(folder + "/coin_spin.mp4"));

    auto& assets = o2EditorAssets;
    auto scroll = assets.GetAssetsScroll();
    assets.OpenFolder(folder);
    shell.Settle();

    auto findIcon = [&](const String& name) {
        for (auto& icon : scroll->GetVisibleIcons())
        {
            if (icon->GetAssetInfo()->path == folder + "/" + name)
                return icon;
        }

        return Ref<AssetIcon>();
    };

    auto video = findIcon("coin_spin.mp4");
    ASSERT_TRUE(video);
    auto frame = MediaPreview::GetVideoThumbnail(AssetRef<VideoAsset>(folder + "/coin_spin.mp4"));
    ASSERT_TRUE(frame);
    auto frameSprite = DynamicCast<Sprite>(video->layer["icon"]->GetDrawable());
    ASSERT_TRUE(frameSprite);
    EXPECT_EQ(frameSprite->GetTexture(), frame);
    RectF frameRect = video->layer["icon"]->GetRect();
    EXPECT_NEAR(frameRect.Width()/frameRect.Height(), (float)frame->GetSize().x/(float)frame->GetSize().y, 0.05f);

    auto badge = video->FindLayer("mediaBadge");
    ASSERT_TRUE(badge && badge->IsEnabled());
    auto fill = badge->FindChild("fill");
    ASSERT_TRUE(fill);
    Color4 outline = badge->GetDrawable()->GetColor();
    EXPECT_TRUE(outline.r == 255 && outline.g == 255 && outline.b == 255) << "the badge has no light outline";
    RectF outlineRect = badge->GetRect(), fillRect = fill->GetRect();
    EXPECT_GT(outlineRect.Width(), fillRect.Width() + 2.0f);
    EXPECT_TRUE(outlineRect.IsInside(fillRect.Center()));

    for (int mode : { 0, 1, 2 })
    {
        assets.SetViewMode(mode);
        shell.Settle();

        auto voice = findIcon("voice.wav");
        ASSERT_TRUE(voice) << "mode " << mode;
        auto soundIcon = DynamicCast<VectorSprite>(voice->layer["icon"]->GetDrawable());
        ASSERT_TRUE(soundIcon && voice->layer["icon"]->IsEnabled()) << "mode " << mode;
        EXPECT_EQ(soundIcon->GetImageName(), String("ui/UI4_big_sound_file_icon.svg")) << "mode " << mode;
        EXPECT_FALSE(voice->GetChildByType<Button>("mediaPlay")) << "a sound plays on its icon, mode " << mode;
        EXPECT_FALSE(voice->FindLayer("playProgress")) << "mode " << mode;
    }
}
