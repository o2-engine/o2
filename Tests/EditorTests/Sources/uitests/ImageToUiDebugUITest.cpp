#include "o2Editor/stdafx.h"
#include <chrono>
#include <set>
#include <thread>
#include "PipelinePairUiFixture.h"
#include "EditorVectorUIFixture.h"

#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Render/Text.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/UI/Style/EditorThemeDark.h"
#include "o2Editor/Windows/PipelineWindow/ImageToUiDebugDlg.h"
#include "o2Editor/Windows/PipelineWindow/ImageToUiEditsDlg.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

// The steps window of AI image to UI: a pick keeps the list where it is, a growing trace keeps both sides where they are,
// the texts take the editor theme, Escape closes the zoom and then the window

namespace
{
    struct ImageToUiDebugUiFixture: PipelinePairUiFixture
    {
        String WorkDir() const override { return "../../Work/Pipelines/uitest-imagetoui-debug/"; }
    };

    // A trace of a run: the design, the analysis, then agent turns with a picture each
    String TraceJson(int turns)
    {
        String steps = "{\"id\":\"input\",\"title\":\"Design\",\"kind\":\"input\",\"images\":[{\"title\":\"design\",\"file\":\"001.png\",\"w\":64,\"h\":96}]},"
                       "{\"id\":\"analysis\",\"title\":\"Analysis: 9 elements, 3 looks\",\"kind\":\"analysis\",\"ms\":1200,\"text\":\"{}\"}";
        for (int i = 1; i <= turns; i++)
        {
            steps += ",{\"id\":\"turn-" + (String)i + "\",\"title\":\"Turn " + (String)i + ": check\",\"kind\":\"check\",\"ms\":900,"
                "\"note\":\"Measured the grid again and moved the tiles\",\"text\":\"ok\",\"data\":{\"action\":\"{\\\"action\\\":\\\"check\\\"}\"},"
                "\"images\":[{\"title\":\"comparison.png\",\"file\":\"001.png\",\"w\":64,\"h\":96}]}";
        }
        return "{\"version\":1,\"design\":{\"file\":\"001.png\",\"width\":64,\"height\":96},\"steps\":[" + steps + "],\"summary\":{}}";
    }
}

TEST_F(ImageToUiDebugUiFixture, TheStepsWindowKeepsItsPlaceAndReadsOnTheDarkTheme)
{
    struct LightAfter
    {
        ~LightAfter()
        {
            ImageToUiDebug::Reset();
            EditorTheme::SetKind(EditorThemeKind::Light);
            EditorVectorUITest::BuildStyle(false);
            PipelineTheme::FollowEditor();
        }
    } lightAfter;

    EditorTheme::SetKind(EditorThemeKind::Dark);
    EditorVectorUITest::BuildStyle(false);
    PipelineTheme::FollowEditor();
    ImageToUiDebug::Reset();

    auto node = OpenFed("imageToUi");
    String trace = PipelineExecutor::GetWorkPath(editor->GetPipelineId(), node->id) + "trace/";
    o2FileSystem.FolderCreate(trace, true);
    PipelineUtils::WriteFileBytes(trace + "001.png", PipelineValue::Image(PipelineImageOps::Blank(64, 96, inputColor)).GetPngBytes());
    PipelineUtils::WriteFileBytes(trace + "trace.json", TraceJson(60));

    ImageToUiDebug::Show(editor, node, {});
    Step(6);
    auto window = ImageToUiDebug::GetWindow();
    auto list = ImageToUiDebug::GetStepsList();
    ASSERT_TRUE(window && list);
    ASSERT_EQ(ImageToUiDebug::GetStepIds().Count(), 62);

    // The texts take the dark editor theme
    auto row = list->FindChildByTypeAndName<Button>("step turn-3");
    ASSERT_TRUE(row);
    auto title = DynamicCast<Text>(row->GetLayer("title")->GetDrawable());
    ASSERT_TRUE(title);
    EXPECT_EQ(Color4(title->GetColor().r, title->GetColor().g, title->GetColor().b, 255), EditorThemeDark::text);
    auto summary = DynamicCast<Label>(window->FindChild("debug summary"));
    ASSERT_TRUE(summary);
    EXPECT_EQ(Color4(summary->GetColor().r, summary->GetColor().g, summary->GetColor().b, 255), EditorThemeDark::text);

    // A pick halfway down the list leaves the list where it is
    RectF range = list->GetScrollRange();
    Vec2F middle(0, (range.bottom + range.top)*0.5f);
    list->SetScrollForcible(middle);
    Step(3);
    Vec2F before = list->GetScroll();
    ASSERT_NE(before.y, 0.0f) << "the list scrolls";
    ImageToUiDebug::SelectStep("turn-30");
    Step(4);
    EXPECT_NEAR(list->GetScroll().y, before.y, 0.5f) << "a pick keeps the list in place";
    EXPECT_TRUE(row->GetLayer("selected") && !row->GetLayer("selected")->enabled);
    EXPECT_TRUE(list->FindChildByTypeAndName<Button>("step turn-30")->GetLayer("selected")->enabled);
    EXPECT_EQ(list->FindChildByTypeAndName<Button>("step turn-3"), row) << "the rows are kept, not built anew";

    // The run goes on: the trace grows, the list and the step shown stay where they are
    auto viewScroll = DynamicCast<ScrollArea>(window->FindChild("debug step view"));
    ASSERT_TRUE(viewScroll);
    viewScroll->SetScrollForcible(Vec2F(0, (viewScroll->GetScrollRange().bottom + viewScroll->GetScrollRange().top)*0.5f));
    Step(2);
    Vec2F viewBefore = viewScroll->GetScroll();
    PipelineUtils::WriteFileBytes(trace + "trace.json", TraceJson(70));
    ImageToUiDebug::Refresh(node->id);
    Step(4);
    EXPECT_EQ(ImageToUiDebug::GetStepIds().Count(), 72);
    EXPECT_NEAR(list->GetScroll().y, before.y, 0.5f) << "new steps keep the list in place";
    EXPECT_NEAR(viewScroll->GetScroll().y, viewBefore.y, 0.5f) << "the step read keeps its place";
    EXPECT_TRUE(list->FindChildByTypeAndName<Button>("step turn-30")->GetLayer("selected")->enabled);

    // Escape with the cursor over the window closes the zoom, then the window
    Vec2F over = window->layout->GetWorldRect().Center();
    o2Input.OnCursorMoved(over, 0, false);
    Step(2);

    String shots = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots"));
    o2FileSystem.FolderCreate(shots, true);
    if (auto frame = Capture())
        frame->Save(shots + "/image_to_ui_debug_dark.png", Bitmap::ImageType::Png);

    ImageToUiDebug::Zoom(0);
    Step(2);
    auto zoom = window->FindChild("debug zoom");
    ASSERT_TRUE(zoom && zoom->IsEnabled());
    o2Input.OnKeyPressed(VK_ESCAPE);
    Step(2);
    o2Input.OnKeyReleased(VK_ESCAPE);
    Step(2);
    EXPECT_FALSE(zoom->IsEnabled()) << "Escape closes the zoom first";
    EXPECT_TRUE(ImageToUiDebug::IsShownFor(node->id));
    o2Input.OnKeyPressed(VK_ESCAPE);
    Step(2);
    o2Input.OnKeyReleased(VK_ESCAPE);
    Step(2);
    EXPECT_FALSE(ImageToUiDebug::IsShownFor(node->id)) << "then the window";
}

// Typing a note of the edits window: the field keeps the focus while the sync saves and reloads around it, the draft goes to the
// node at once but tells the editor (and so the save and the sync) only when the field is left
TEST_F(ImageToUiDebugUiFixture, TypingANoteKeepsItsFieldWhileTheSyncReloads)
{
    struct Close { ~Close() { if (auto window = ImageToUiEdits::GetWindow()) window->Hide(true); } } close;
    auto node = OpenFed("imageToUi");
    ImageToUiEdits::Show(editor, node, {});
    Step(4);
    ImageToUiEdits::AddGeneralNote();
    Step(4);

    auto window = ImageToUiEdits::GetWindow();
    ASSERT_TRUE(window);
    auto field = [&]() { return window->FindChildByTypeAndName<EditBox>("note text"); };
    ASSERT_TRUE(field());
    EXPECT_TRUE(field()->IsFocused()) << "a new note takes the focus";
    asset->SetDirty(false);

    // Typing
    auto edit = field();
    for (String text : { "M", "Ma", "Make the button green" })
    {
        edit->SetText(text);
        edit->onChanged(edit->GetText());
        Step(1);
    }
    edit->SetCaretPosition(21);
    EXPECT_EQ(ImageToUiEdits::GetDraft()[0].text, "Make the button green");
    EXPECT_EQ(PipelineImageToUi::ReadDraft(*editor->GetGraph()->FindNode(node->id))[0].text, "Make the button green") << "the node has it";
    EXPECT_FALSE(asset->IsDirty()) << "typing does not save the pipeline every second";

    // What a sync pass does around it: the card follows its config and outputs, the results and the graph are read again
    ImageToUiEdits::Refresh(node->id);
    editor->ReloadResults();
    editor->ReloadGraph();
    Step(4);
    ASSERT_TRUE(field());
    EXPECT_TRUE(field()->IsFocused()) << "the field keeps the focus";
    EXPECT_EQ((String)field()->GetText(), "Make the button green");
    EXPECT_EQ(field()->GetCaretPosition(), 21);
    EXPECT_EQ(ImageToUiEdits::GetDraft()[0].text, "Make the button green");

    // Leaving the field tells the editor: then the pipeline saves and syncs
    field()->onChangeCompleted(field()->GetText());
    EXPECT_TRUE(asset->IsDirty());
}

// The steps window over a recorded trace, for review (O2_IMAGE_TO_UI_TRACE_DIR = a copy of a node's trace folder): a shot of the
// first step of every kind and of a few agent turns, and how long each pick takes
TEST_F(ImageToUiDebugUiFixture, GalleryOfARecordedTrace)
{
    const char* source = getenv("O2_IMAGE_TO_UI_TRACE_DIR");
    if (!source)
        GTEST_SKIP() << "O2_IMAGE_TO_UI_TRACE_DIR is not set";
    String out = getenv("O2_IMAGE_TO_UI_GALLERY_OUT") ? String(getenv("O2_IMAGE_TO_UI_GALLERY_OUT")) : String("../../Work/Pipelines/shots/debug");
    o2FileSystem.FolderCreate(out, true);

    struct Reset { ~Reset() { ImageToUiDebug::Reset(); } } reset;
    ImageToUiDebug::Reset();
    auto node = OpenFed("imageToUi");
    String trace = PipelineExecutor::GetWorkPath(editor->GetPipelineId(), node->id) + "trace/";
    o2FileSystem.FolderCreate(trace, true);
    for (auto& file : o2FileSystem.GetFolderInfo(source).files)
    {
        String name = file.path.SubStr(file.path.FindLast("/") + 1);
        PipelineUtils::WriteFileBytes(trace + name, PipelineUtils::ReadFileBytes(String(source) + "/" + name));
    }

    ImageToUiDebug::Show(editor, node, {});
    Step(6);
    auto ids = ImageToUiDebug::GetStepIds();
    ASSERT_FALSE(ids.IsEmpty());

    UiLayout::Json parsed;
    ASSERT_TRUE(UiLayout::Json::Parse(PipelineUtils::ReadFileBytes(trace + "trace.json"), parsed));
    std::set<std::string> kinds;
    int turns = 0, index = 0;
    for (auto& step : parsed.Get("steps")->Items())
    {
        String id = step.Get("id")->Str(), kind = step.Get("kind") ? step.Get("kind")->Str() : String();
        bool turn = id.StartsWith("turn-");
        if (!kinds.insert(kind.Data()).second && !(turn && turns < 3))
        {
            index++;
            continue;
        }
        if (turn)
            turns++;

        auto start = std::chrono::steady_clock::now();
        ImageToUiDebug::SelectStep(id);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        for (int i = 0; i < 40; i++)
        {
            Step(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        printf("pick %s (%s): %.1f ms\n", id.Data(), kind.Data(), ms);
        if (auto frame = Capture())
            frame->Save(out + "/" + (String)index + "-" + kind + "-" + id + ".png", Bitmap::ImageType::Png);
        index++;
    }
}
