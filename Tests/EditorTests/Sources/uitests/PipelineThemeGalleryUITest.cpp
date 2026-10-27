#include "o2Editor/stdafx.h"
#include "PipelinePairUiFixture.h"

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2Editor/Windows/PipelineWindow/PipelineLayoutDump.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

// Shoots every card of an AssetsLine export at zoom 1, where the web reference shot has it on screen, with a block dump
// next to each frame; scratch tooling compares the two editors pixel by pixel and block by block. Gated: it needs the
// export (O2_THEME_GALLERY_BUNDLE) and writes into O2_THEME_GALLERY_OUT; O2_THEME_GALLERY_REF points at the web dumps

namespace
{
    String Env(const char* name, const String& def = "")
    {
        const char* value = getenv(name);
        return value ? String(value) : def;
    }

    // Top-left of the card on the web shot, in window pixels from the top-left corner
    bool ReadWebPlacement(const String& refDir, const String& type, Vec2F& topLeft, Vec2I& viewport)
    {
        String path = refDir + "/" + type + ".json";
        DataDocument doc;
        if (refDir.IsEmpty() || !o2FileSystem.IsFileExist(path) || !doc.LoadFromFile(path) || !doc.IsObject())
            return false;

        auto screen = doc.FindMember("cardScreen");
        if (!screen || !screen->IsObject())
            return false;

        topLeft = Vec2F((float)(*screen)["x"], (float)(*screen)["y"]);
        viewport = Vec2I(1600, 1000);
        return true;
    }
}

TEST(PipelineThemeGallery, ShootsEveryCard)
{
    String bundlePath = Env("O2_THEME_GALLERY_BUNDLE");
    if (bundlePath.IsEmpty())
        GTEST_SKIP() << "O2_THEME_GALLERY_BUNDLE is not set";

    String outDir = Env("O2_THEME_GALLERY_OUT", "../../Work/Pipelines/shots/theme");
    String refDir = Env("O2_THEME_GALLERY_REF");
    String only = Env("O2_THEME_GALLERY_ONLY");
    bool light = Env("O2_THEME", "dark") == "light";
    o2FileSystem.FolderCreate(outDir, true);

    if (!UIRoot::IsSingletonInitialzed())
    {
        PushEditorScopeOnStack scope;
        mmake<UIRoot>();
    }
    o2Application.SetContentSize(Vec2I(1600, 1000));
    Step(3);
    PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest-theme/");
    o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
    o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
    PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
    PipelineTheme::SetKind(light ? PipelineThemeKind::Light : PipelineThemeKind::Dark);

    auto bundle = PipelineImport::ParseBytes(PipelineUtils::ReadFileBytes(bundlePath));
    ASSERT_TRUE(bundle.ok) << bundle.error;
    PipelineImport::StoreResults(bundle.graph.id, bundle);

    Ref<PipelineEditor> editor;
    {
        PushEditorScopeOnStack scope;
        editor = mmake<PipelineEditor>();
        *editor->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
        EditorUIRoot.AddWidget(editor);
        editor->SetEnabledForcible(false);
        editor->SetEnabledForcible(true);
        for (int i = 0; i < 12; i++)
        {
            root->Update(0.1f);
            root->UpdateChildren(0.1f);
            root->UpdateChildrenTransforms();
        }
    }
    editor->drawEdges = false;

    auto asset = mmake<PipelineAsset>();
    bundle.graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    o2Input.OnCursorMoved(Vec2F(-5000, -5000), 0, false);
    Step(20);
    editor->FinishBackgroundWork();
    Step(4);

    Vec2I content = o2Application.GetContentSize();
    int shot = 0;
    for (auto& node : editor->GetGraph()->nodes)
    {
        String type = node->nodeType;
        if (!only.IsEmpty() && !("," + only + ",").Contains("," + type + ","))
            continue;

        auto card = editor->GetNodeWidget(node->id);
        ASSERT_TRUE(card) << type;

        Vec2F webTopLeft;
        Vec2I viewport;
        RectF rect = card->GetCardRect();
        if (!ReadWebPlacement(refDir, type, webTopLeft, viewport))
            webTopLeft = Vec2F(Math::Round(content.x * 0.5f - rect.Width() * 0.5f), Math::Round(content.y * 0.5f - rect.Height() * 0.5f));

        editor->SetView(rect.Center(), 1.0f);
        Step(2);
        Vec2F current = editor->LocalToScreenPoint(Vec2F(rect.left, rect.top));
        Vec2F wanted(-content.x * 0.5f + webTopLeft.x, content.y * 0.5f - webTopLeft.y);
        editor->SetView(rect.Center() + (current - wanted), 1.0f);
        Step(2);
        editor->FinishBackgroundWork();
        Step(12);

        auto frame = Capture();
        ASSERT_TRUE(frame) << type;
        frame->Save(outDir + "/" + type + ".frame.png", Bitmap::ImageType::Png);

        DataDocument doc;
        PipelineLayoutDump::Dump(*card, doc);
        doc["type"] = type;
        doc["nodeId"] = node->id;
        doc["theme"] = String(light ? "light" : "dark");
        Vec2F placed = editor->LocalToScreenPoint(Vec2F(rect.left, rect.top));
        doc["cardScreen"].SetObject();
        doc["cardScreen"]["x"] = placed.x + content.x * 0.5f;
        doc["cardScreen"]["y"] = content.y * 0.5f - placed.y;
        doc["cardScreen"]["w"] = rect.Width();
        doc["cardScreen"]["h"] = rect.Height();
        doc["frame"].SetObject();
        doc["frame"]["w"] = frame->GetSize().x;
        doc["frame"]["h"] = frame->GetSize().y;
        doc["viewport"].SetObject();
        doc["viewport"]["w"] = content.x;
        doc["viewport"]["h"] = content.y;
        doc.SaveToFile(outDir + "/" + type + ".json");
        shot++;
    }
    EXPECT_GT(shot, 0);

    // The whole graph with its links, for an overview of the theme
    editor->drawEdges = true;
    editor->FitView();
    Step(2);
    editor->FinishBackgroundWork();
    Step(12);
    if (auto overview = Capture())
        overview->Save(outDir + "/_overview.frame.png", Bitmap::ImageType::Png);

    {
        PushEditorScopeOnStack scope;
        EditorUIRoot.RemoveAllWidgets();
    }
    editor = nullptr;
    PipelineTheme::SetKind(PipelineThemeKind::Dark);
    o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
    PipelineUtils::SetWorkPathOverride("");
}
