#include "o2Editor/stdafx.h"
#include "PipelinePairUiFixture.h"

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

// Shoots every card of an AssetsLine export close up and zoomed out past the detail level, where a card drops its
// controls but keeps its results; a json next to the frames says where each card is on them. Gated: it needs the export
// (O2_FARVIEW_BUNDLE), or a .pipeline (O2_FARVIEW_PIPELINE) over a work folder with its cache (O2_FARVIEW_WORK), which is
// only shot whole; writes into O2_FARVIEW_OUT; O2_FARVIEW_IO=compare shoots the compare layout

namespace
{
    String FarEnv(const char* name, const String& def = "")
    {
        const char* value = getenv(name);
        return value ? String(value) : def;
    }

    void SaveCardPlacement(DataValue& out, const Ref<PipelineEditor>& editor, const RectF& rect, const Ref<Bitmap>& frame)
    {
        Vec2I content = o2Application.GetContentSize();
        float k = (float)frame->GetSize().x/Math::Max(1, content.x);
        Vec2F topLeft = editor->LocalToScreenPoint(Vec2F(rect.left, rect.top));
        Vec2F bottomRight = editor->LocalToScreenPoint(Vec2F(rect.right, rect.bottom));
        out.SetObject();
        out["x"] = (topLeft.x + content.x*0.5f)*k;
        out["y"] = (content.y*0.5f - topLeft.y)*k;
        out["w"] = (bottomRight.x - topLeft.x)*k;
        out["h"] = (topLeft.y - bottomRight.y)*k;
    }
}

TEST(PipelineFarView, ShootsEveryCardCloseAndFar)
{
    String bundlePath = FarEnv("O2_FARVIEW_BUNDLE");
    String pipelinePath = FarEnv("O2_FARVIEW_PIPELINE");
    String workPath = FarEnv("O2_FARVIEW_WORK");
    if (bundlePath.IsEmpty() && (pipelinePath.IsEmpty() || workPath.IsEmpty()))
        GTEST_SKIP() << "neither O2_FARVIEW_BUNDLE nor O2_FARVIEW_PIPELINE with O2_FARVIEW_WORK is set";
    bool whole = bundlePath.IsEmpty();

    String outDir = FarEnv("O2_FARVIEW_OUT", "../../Work/Pipelines/shots/farview");
    String only = FarEnv("O2_FARVIEW_ONLY");
    bool compare = FarEnv("O2_FARVIEW_IO") == "compare";
    float farScale = (float)atof(FarEnv("O2_FARVIEW_SCALE", "5").Data());
    o2FileSystem.FolderCreate(outDir, true);

    if (!UIRoot::IsSingletonInitialzed())
    {
        PushEditorScopeOnStack scope;
        mmake<UIRoot>();
    }
    o2Application.SetContentSize(Vec2I(1600, 1000));
    Step(3);
    PipelineUtils::SetWorkPathOverride(whole ? (workPath.EndsWith("/") ? workPath : workPath + "/") : String("../../Work/Pipelines/uitest-farview/"));
    if (!whole)
    {
        o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
        o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
    }
    PipelinePairLayout::SetIoView(compare ? PipelineIoView::Compare : PipelineIoView::SideBySide);
    PipelineTheme::SetKind(PipelineThemeKind::Dark);

    auto asset = mmake<PipelineAsset>();
    if (whole)
        ASSERT_TRUE(asset->document.LoadFromFile(pipelinePath)) << pipelinePath.Data();
    else
    {
        auto bundle = PipelineImport::ParseBytes(PipelineUtils::ReadFileBytes(bundlePath));
        ASSERT_TRUE(bundle.ok) << bundle.error;
        PipelineImport::StoreResults(bundle.graph.id, bundle);
        bundle.graph.SaveToAsset(*asset);
    }

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
    editor->drawEdges = whole;
    editor->SetAsset(asset);
    o2Input.OnCursorMoved(Vec2F(-5000, -5000), 0, false);

    // Opened zoomed out: the pictures arrive while every card is without details
    RectF bounds;
    bool first = true;
    for (auto& node : editor->GetGraph()->nodes)
    {
        RectF rect = editor->GetNodeWidget(node->id)->GetCardRect();
        bounds = first ? rect : RectF(Math::Min(bounds.left, rect.left), Math::Max(bounds.top, rect.top),
                                      Math::Max(bounds.right, rect.right), Math::Min(bounds.bottom, rect.bottom));
        first = false;
    }
    editor->SetView(bounds.Center(), farScale);
    Step(20);
    editor->FinishBackgroundWork();
    Step(12);

    // Video previews and waveforms are read in threads of their own; the cards apply them without details too
    auto loading = [&]()
    {
        String names;
        for (auto& node : editor->GetGraph()->nodes)
        {
            auto card = editor->GetNodeWidget(node->id);
            if (card && card->GetBody() && card->GetBody()->IsLoadingMedia())
                names += (names.IsEmpty() ? "" : ", ") + node->nodeType + " " + node->id;
        }
        return names;
    };
    // A long track's waveform takes a minute in a debug build: a whole pipeline is shot without waiting for it
    for (int i = 0; i < 1500 && !loading().IsEmpty(); i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        Step();
    }
    if (!whole)
        EXPECT_EQ(loading(), String()) << "media still loading";
    Step(2);

    String suffix = compare ? ".compare" : "";
    if (auto opened = Capture())
        opened->Save(outDir + "/_opened_far" + suffix + ".png", Bitmap::ImageType::Png);

    // A whole pipeline: fitted, then its middle past the detail level and close
    Vec2I content = o2Application.GetContentSize();
    float fit = Math::Max(bounds.Width()/content.x, bounds.Height()/content.y)*1.05f;
    for (auto& shot : Vector<Pair<String, float>>{ { "_whole", fit }, { "_middle_far", farScale }, { "_middle_close", 2.0f } })
    {
        if (!whole)
            break;

        editor->SetView(bounds.Center(), shot.second);
        Step(4);
        editor->FinishBackgroundWork();
        Step(12);
        if (auto frame = Capture())
            frame->Save(outDir + "/" + shot.first + suffix + ".png", Bitmap::ImageType::Png);
    }

    DataDocument index;
    index.SetObject();
    int shot = 0;
    for (auto& node : editor->GetGraph()->nodes)
    {
        String type = node->nodeType;
        if (whole || (!only.IsEmpty() && !("," + only + ",").Contains("," + type + ",")))
            continue;

        auto card = editor->GetNodeWidget(node->id);
        ASSERT_TRUE(card) << type;
        RectF rect = card->GetCardRect();
        auto& entry = index[type];
        entry.SetObject();

        for (int pass = 0; pass < 2; pass++)
        {
            bool far = pass == 0;
            editor->SetView(rect.Center(), far ? farScale : 1.0f);
            Step(2);
            editor->FinishBackgroundWork();
            Step(12);
            EXPECT_EQ(card->IsDetailed(), far == 0) << type.Data();

            auto frame = Capture();
            ASSERT_TRUE(frame) << type;
            String name = type + (far ? ".far" : ".close") + suffix + ".png";
            frame->Save(outDir + "/" + name, Bitmap::ImageType::Png);
            SaveCardPlacement(entry[far ? "far" : "close"], editor, rect, frame);
        }
        shot++;
    }
    EXPECT_TRUE(whole || shot > 0);
    index.SaveToFile(outDir + "/index" + suffix + ".json");

    // The whole graph fitted, links included
    editor->drawEdges = true;
    editor->FitView();
    Step(2);
    editor->FinishBackgroundWork();
    Step(12);
    if (auto overview = Capture())
        overview->Save(outDir + "/_overview" + suffix + ".png", Bitmap::ImageType::Png);

    {
        PushEditorScopeOnStack scope;
        EditorUIRoot.RemoveAllWidgets();
    }
    editor = nullptr;
    PipelineTheme::FollowEditor();
    PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
    if (!whole)
        o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
    PipelineUtils::SetWorkPathOverride("");
}
