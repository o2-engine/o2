#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Windows/PipelineWindow/PipelineCheckDlg.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePartsGrid.h"

// The checks against the reference on the cards: the AI extract's check rows, the third button on a part and its debug popup,
// a part's status while it is checked or drawn again; the AI text style card - the source with its frame, the sample, the
// summary line, "-> Text style" as one undo step, its check rows and popup. Run with O2_INVISIBLE_WINDOW=1

namespace
{
    using UiLayout::Json;

    struct PipelineChecksFixture : PipelinePairUiFixture
    {
        Ref<ActionsList> actions;

        String WorkDir() const override { return "../../Work/Pipelines/uitest-checks/"; }

        void TearDown() override
        {
            PipelineCheckDlg::Reset();
            PipelinePairUiFixture::TearDown();
        }

        String CheckFolder(const Ref<PipelineNode>& node) const
        {
            return PipelineExtractCheck::CheckFolder(PipelineExecutor::GetWorkPath(editor->GetPipelineId(), node->id));
        }

        void WritePicture(const String& path, int w, int h, const Color4& color)
        {
            PipelineUtils::WriteFileBytes(path, PipelineValue::Image(PipelineImageOps::Blank(w, h, color)).GetPngBytes());
        }

        // A finished check of a part: a rejected first attempt and an accepted regeneration
        void WritePartCheck(const Ref<PipelineNode>& node, const String& partId)
        {
            String folder = CheckFolder(node);
            String tag = PipelineExtractCheck::PartFile(partId) + "-0123abcd";
            WritePicture(folder + tag + "-ref.png", 384, 320, Color4(51, 102, 204, 255));
            for (int n = 1; n <= 2; n++)
            {
                WritePicture(folder + tag + "-a" + (String)n + "-sprite.png", 80, 80, Color4(208, 32, 32, 255));
                WritePicture(folder + tag + "-a" + (String)n + "-overlay.png", 384, 320, Color4(120, 80, 160, 255));
            }
            String json = "{\"version\":1,\"key\":\"0123abcd\",\"part\":{\"id\":\"" + partId + "\",\"name\":\"red square\"},\"at\":\"2026-11-12T10:00:00.000Z\","
                "\"ms\":5300,\"limit\":2,\"judge\":\"gpt-6.1-sol\",\"reference\":\"" + tag + "-ref.png\",\"referenceSize\":{\"w\":384,\"h\":320},"
                "\"regionInReference\":{\"x\":64,\"y\":64,\"w\":256,\"h\":192},\"attempts\":["
                "{\"n\":1,\"sprite\":\"" + tag + "-a1-sprite.png\",\"overlay\":\"" + tag + "-a1-overlay.png\",\"spriteSize\":{\"w\":52,\"h\":80},"
                "\"placement\":{\"x\":80,\"y\":30,\"w\":26,\"h\":40,\"from\":\"tracked\"},\"pixelDiff\":0.21,\"pixelChanged\":0.4,\"verdict\":\"significant\","
                "\"likeness\":0.4,\"differences\":[\"the right edge is cut off\"],\"instructions\":\"Keep the whole square.\",\"clarifications\":\"\","
                "\"model\":\"gemini-3.1-flash-image\",\"judge\":\"gpt-6.1-sol\",\"drawMs\":0,\"judgeMs\":1200},"
                "{\"n\":2,\"sprite\":\"" + tag + "-a2-sprite.png\",\"overlay\":\"" + tag + "-a2-overlay.png\",\"spriteSize\":{\"w\":80,\"h\":80},"
                "\"placement\":{\"x\":80,\"y\":30,\"w\":40,\"h\":40,\"from\":\"tracked\",\"fit\":{\"scale\":1,\"dx\":-6,\"dy\":0}},\"pixelDiff\":0.01,"
                "\"pixelChanged\":0.02,\"verdict\":\"ok\",\"likeness\":0.97,\"differences\":[],\"instructions\":\"\",\"clarifications\":\"IMPORTANT - rejected\","
                "\"model\":\"gemini-3.1-flash-image\",\"judge\":\"gpt-6.1-sol\",\"drawMs\":3100,\"judgeMs\":1000}],"
                "\"kept\":2,\"stop\":\"ok\",\"note\":\"\",\"calls\":{\"judge\":2,\"draw\":1}}";
            PipelineUtils::WriteFileBytes(folder + PipelineExtractCheck::PartFile(partId) + ".json", json);
        }

        // Saves the whole window: the popup over the editor
        void ShotScreen(const String& name)
        {
            Step(30);
            auto capture = Capture();
            ASSERT_TRUE(capture);
            String dir = (getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots")) + "/pairs";
            o2FileSystem.FolderCreate(dir, true);
            EXPECT_TRUE(capture->Save(dir + "/" + name + ".png", Bitmap::ImageType::Png));
        }

        Ref<Toggle> FindByName(const Ref<Widget>& root, const String& name)
        {
            return root->FindChildByTypeAndName<Toggle>(name);
        }
    };

    const char* styleJson = "{\"name\":\"title_gold\",\"role\":\"title\",\"font\":\"Russo One\",\"weight\":400,\"size\":120,\"color\":\"#FFE45C\","
        "\"fill\":{\"angle\":90,\"stops\":[{\"color\":\"#FFF6B0\",\"at\":0},{\"color\":\"#FF9A00\",\"at\":1}]},"
        "\"effects\":[{\"type\":\"stroke\",\"color\":\"#3A1800\",\"width\":8,\"opacity\":1},{\"type\":\"shadow\",\"color\":\"#000000\",\"x\":0,\"y\":6,\"blur\":4,"
        "\"spread\":0,\"opacity\":0.5}],\"letterSpacing\":0,\"lineHeight\":1.2,\"uppercase\":true,\"align\":\"center\","
        "\"extracted\":{\"name\":\"title_gold\",\"role\":\"title\",\"text\":\"QUEST\"}}";
}

TEST_F(PipelineChecksFixture, ExtractCardCheckRowsPartButtonAndPopup)
{
    auto node = OpenFed("imageExtract", 480.0f, true, PipelineImageOps::Blank(40, 40, resultColor), 2);
    node->SetConfigBool("paramsOpen", true);
    editor->GetNodeWidget(node->id)->Rebuild();
    Step(3);
    auto card = editor->GetNodeWidget(node->id);

    // Off: the switch alone, no check button on the parts
    auto toggle = FindByName(card, "ref check");
    ASSERT_TRUE(toggle);
    EXPECT_FALSE(toggle->GetValue());
    EXPECT_FALSE(card->FindChildByTypeAndName<Button>("check details"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Widget>("ref check max"));

    // On: the limit and the judge show, every part gets the button
    toggle->onToggleByUser(true);
    Step(3);
    card = editor->GetNodeWidget(node->id);
    EXPECT_TRUE(PipelineGraph::IsRefCheckOn(*editor->GetGraph()->FindNode(node->id)));
    ASSERT_TRUE(card->FindChildByTypeAndName<Widget>("ref check max"));
    auto limit3 = FindByName(card, "limit 3");
    ASSERT_TRUE(limit3);
    limit3->onToggleByUser(true);
    Step(2);
    EXPECT_EQ(editor->GetGraph()->FindNode(node->id)->GetConfigNumber("refCheckMax"), 3.0f);

    // A part being checked or drawn again says so over its cell
    card = editor->GetNodeWidget(node->id);
    auto grid = card->FindChildByTypeAndName<PipelinePartsGrid>("parts");
    ASSERT_TRUE(grid);
    auto& runtime = card->GetRuntime();
    runtime.state = "running";
    runtime.portStates["part0"] = "running";
    runtime.phases["part0"] = "redo";
    card->ApplyRuntime();
    Step(2);
    EXPECT_EQ(grid->GetPartPhase("part0"), "redrawing");
    EXPECT_EQ(grid->GetPartPhase("part1"), "");
    ShotCard(node, "check_extract_running");
    runtime.state = "idle";
    card->ApplyRuntime();
    Step(2);
    EXPECT_EQ(grid->GetPartPhase("part0"), "");

    // The part's button opens the popup with its attempts side by side
    WritePartCheck(node, "part0");
    grid->SetSelected("part0");
    Step(2);
    auto details = card->FindChildByTypeAndName<Button>("check details");
    ASSERT_TRUE(details);
    details->onClick();
    Step(4);
    EXPECT_TRUE(PipelineCheckDlg::IsShownFor(node->id, "part0"));
    EXPECT_EQ(PipelineCheckDlg::GetAttemptCount(), 2);
    EXPECT_EQ(PipelineCheckDlg::GetStopLine(), "Accepted: the sprite matches the reference.");
    ShotScreen("check_extract_popup");
    PipelineCheckDlg::Hide();
    Step(30);
    ShotCard(node, "check_extract_card");
}

TEST_F(PipelineChecksFixture, TextStyleCardSourceSampleSummaryAndTextStyleNode)
{
    actions = mmake<ActionsList>();
    editor->actionsListDelegate = actions;
    auto node = OpenFed("textStyleExtract", 360.0f, true);
    node->SetConfigBool("paramsOpen", true);
    auto card = editor->GetNodeWidget(node->id);
    card->Rebuild();
    Step(2);

    // Nothing read yet: the source with its frame, the empty line, the hint
    auto source = card->FindChildByTypeAndName<PipelineCropEditor>("style source crop");
    ASSERT_TRUE(source);
    card->OnOutputChanged();
    Step(2);
    EXPECT_TRUE(source->HasImage()) << "the input picture is the source";
    EXPECT_TRUE(card->FindChildByTypeAndName<Label>("style empty"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Button>("to text style"));

    // A style read: the sample, the summary and the action
    card->GetRuntime().output = PipelineValue::TextOf(PipelinePortType::TextStyle, styleJson);
    card->OnOutputChanged();
    Step(3);
    card = editor->GetNodeWidget(node->id);
    auto sample = card->FindChildByTypeAndName<PipelineImageView>("style sample");
    ASSERT_TRUE(sample);
    EXPECT_TRUE(sample->HasImage());
    EXPECT_TRUE(card->FindChildByTypeAndName<Widget>("style summary"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Label>("style empty"));
    ShotCard(node, "check_textstyle_card");

    // A frame on the source; "whole picture" drops it
    node = editor->GetGraph()->FindNode(node->id);
    auto& frame = node->config["srcCrop"];
    frame.SetObject();
    frame["x"] = 0.25f; frame["y"] = 0.25f; frame["w"] = 0.5f; frame["h"] = 0.5f;
    card->Rebuild();
    Step(2);
    auto whole = card->FindChildByTypeAndName<Button>("whole picture");
    ASSERT_TRUE(whole);
    EXPECT_TRUE(whole->IsEnabled());
    whole->onClick();
    Step(2);
    EXPECT_FALSE(editor->GetGraph()->FindNode(node->id)->HasConfig("srcCrop"));

    // "-> Text style": a Text style node with this style beside the card, one undo step
    int before = editor->GetGraph()->nodes.Count();
    auto toStyle = card->FindChildByTypeAndName<Button>("to text style");
    ASSERT_TRUE(toStyle);
    toStyle->onClick();
    Step(3);
    ASSERT_EQ(editor->GetGraph()->nodes.Count(), before + 1);
    auto made = editor->GetGraph()->nodes.Last();
    EXPECT_EQ(made->nodeType, "textStyle");
    EXPECT_EQ(made->GetConfigString("font"), "Russo One");
    EXPECT_EQ(made->GetConfigString("tsSample"), "QUEST");
    EXPECT_FALSE(made->HasConfig("extracted"));
    RectF mine = editor->GetNodeWidget(node->id)->GetCardRect(), theirs = editor->GetNodeWidget(made->id)->GetCardRect();
    EXPECT_GT(theirs.left, mine.right) << "placed to the right";
    actions->UndoAction();
    Step(3);
    EXPECT_EQ(editor->GetGraph()->nodes.Count(), before);

    // The check: its rows, the button on the sample and the popup; "checking..." while it runs
    card = editor->GetNodeWidget(node->id);
    auto toggle = FindByName(card, "ref check");
    ASSERT_TRUE(toggle);
    toggle->onToggleByUser(true);
    Step(3);
    card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(FindByName(card, "limit 2"));
    auto& runtime = card->GetRuntime();
    runtime.state = "running";
    runtime.phases[""] = "check";
    card->ApplyRuntime();
    Step(2);
    auto phase = card->FindChildByTypeAndName<Label>("style phase");
    ASSERT_TRUE(phase);
    EXPECT_TRUE(phase->IsEnabled());
    runtime.state = "idle";
    card->ApplyRuntime();
    Step(2);
    EXPECT_FALSE(phase->IsEnabled());

    String folder = CheckFolder(node);
    WritePicture(folder + "style-0123abcd-ref.png", 540, 400, Color4(32, 48, 80, 255));
    WritePicture(folder + "style-0123abcd-a1.png", 700, 230, Color4(32, 48, 80, 255));
    PipelineUtils::WriteFileBytes(folder + "style.json",
        "{\"version\":1,\"key\":\"0123abcd\",\"at\":\"2026-11-12T10:00:00.000Z\",\"ms\":2100,\"limit\":2,\"judge\":\"gpt-6.1-sol\","
        "\"reference\":\"style-0123abcd-ref.png\",\"text\":\"QUEST\",\"attempts\":[{\"n\":1,\"render\":\"style-0123abcd-a1.png\","
        "\"style\":{\"name\":\"title_gold\",\"font\":\"Russo One\",\"weight\":400,\"size\":120},\"verdict\":\"minor\",\"likeness\":0.86,"
        "\"differences\":[\"the outline is a little thinner\"],\"changes\":[],\"judge\":\"gpt-6.1-sol\",\"judgeMs\":2000}],"
        "\"kept\":1,\"stop\":\"minor\",\"note\":\"\",\"calls\":{\"judge\":1}}");
    auto debug = card->FindChildByTypeAndName<Button>("check details");
    ASSERT_TRUE(debug);
    debug->onClick();
    Step(4);
    EXPECT_TRUE(PipelineCheckDlg::IsShownFor(node->id, ""));
    EXPECT_EQ(PipelineCheckDlg::GetAttemptCount(), 1);
    EXPECT_EQ(PipelineCheckDlg::GetStopLine(), "Small differences only: the style is accepted.");
    ShotScreen("check_textstyle_popup");
    PipelineCheckDlg::Hide();
    Step(30);
    ShotCard(node, "check_textstyle_on");
}
