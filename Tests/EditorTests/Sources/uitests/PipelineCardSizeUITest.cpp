#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

using namespace o2;
using namespace Editor;

// The height of a card: the parameter list opens at once and a card with a set height grows by it, and no card
// gets shorter than its content. Screenshots go to Work/Pipelines/shots/ when run from build/o2

namespace
{
    void Step(int frames = 1)
    {
        for (int i = 0; i < frames; i++)
        {
            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(30, 31, 34, 255));
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f/60.0f);
            root->UpdateChildren(1.0f/60.0f);
            root->UpdateChildrenTransforms();
            root->Draw();
            o2Render.End();
            AppTestDriver::PumpFrames(1);
        }
    }

    void Drag(const Vec2F& from, const Vec2F& to)
    {
        o2Input.OnCursorMoved(from, 0, false);
        o2Input.OnCursorPressed(from);
        Step();
        for (int i = 1; i <= 12; i++)
        {
            o2Input.OnCursorMoved(Math::Lerp(from, to, (float)i/12), 0);
            Step();
        }
        o2Input.OnCursorReleased();
        Step();
    }

    struct PipelineCardSizeUiFixture : ::testing::Test
    {
        Ref<PipelineEditor> editor;
        Ref<PipelineAsset>  asset;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }
            PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest-card-size/");

            PushEditorScopeOnStack scope;
            editor = mmake<PipelineEditor>();
            *editor->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
            auto horScroll = o2UI.CreateHorScrollBar();
            *horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 10);
            editor->SetHorScrollbar(horScroll);
            auto verScroll = o2UI.CreateVerScrollBar();
            *verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 5, 15, 10);
            editor->SetVerScrollbar(verScroll);

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
            asset = mmake<PipelineAsset>();
        }

        void TearDown() override
        {
            editor = nullptr;
            asset = nullptr;
            o2FileSystem.FolderRemove(PipelineUtils::GetWorkPath(), true);
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        Ref<PipelineNode> Open(const Ref<PipelineNode>& node)
        {
            PipelineGraph graph;
            graph.nodes.Add(node);
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(3);
            auto card = editor->GetNodeWidget(node->id);
            editor->SetView(card->GetCardRect().Center(), 1.0f);
            Step(3);
            return editor->GetGraph()->FindNode(node->id);
        }
    };
}

// Like AssetsLine: a card with a set height grows by exactly the list, so the result keeps its size, and folding
// the list gives the height back
TEST_F(PipelineCardSizeUiFixture, ParametersGrowASizedCardByTheirHeight)
{
    auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    gen->size = Vec2F(260, 420);
    gen = Open(gen);
    auto card = editor->GetNodeWidget(gen->id);
    ASSERT_TRUE(card);
    ASSERT_GT(420.0f, card->GetAutoHeight()) << "the card needs some slack for the result to stretch into";

    float shown = card->GetCardSize().y;
    float autoBefore = card->GetAutoHeight();
    auto result = card->GetBody()->mCropEditor;
    ASSERT_TRUE(result);
    float resultBefore = result->layout->GetHeight();

    card->FindChildByTypeAndName<Button>("params")->onClick();
    Step(2);
    EXPECT_TRUE(card->FindChildByTypeAndName<Button>("model")) << "the rows show at once";
    float grown = card->GetAutoHeight() - autoBefore;
    EXPECT_GT(grown, 40.0f);
    EXPECT_NEAR(gen->size.y, shown + grown, 0.5f);
    EXPECT_NEAR(card->GetCardSize().y, shown + grown, 0.5f);
    EXPECT_NEAR(card->GetBody()->mCropEditor->layout->GetHeight(), resultBefore, 1.0f) << "the result keeps its size";

    String dir = getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots");
    Ref<Bitmap> shot;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
    Step();
    ASSERT_TRUE(shot);
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(shot->Save(dir + "/pipeline_sized_card_params_open.png", Bitmap::ImageType::Png));

    card->FindChildByTypeAndName<Button>("params")->onClick();
    Step(2);
    EXPECT_FALSE(card->FindChildByTypeAndName<Button>("model"));
    EXPECT_NEAR(gen->size.y, shown, 0.5f);
    EXPECT_NEAR(card->GetCardSize().y, shown, 0.5f);
    EXPECT_NEAR(card->GetBody()->mCropEditor->layout->GetHeight(), resultBefore, 1.0f);
}

// A model that renders the alpha itself leaves nothing to set: the method and the key rows give way to a note,
// and come back, with the values they had, once another model is chosen
TEST_F(PipelineCardSizeUiFixture, AModelThatRendersTheAlphaHidesTheMethodRows)
{
    auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    gen->SetConfigString("model", "gpt-image-1");
    gen->SetConfigBool("transparentBg", true);
    gen->SetConfigString("transparentMode", "chroma");
    gen->SetConfigNumber("chromaTolerance", 45.0f);
    gen->SetConfigBool("paramsOpen", true);
    gen = Open(gen);
    auto card = editor->GetNodeWidget(gen->id);
    ASSERT_TRUE(card);

    auto note = card->FindChildByTypeAndName<Label>("native transparency");
    ASSERT_TRUE(note);
    EXPECT_EQ((String)note->GetText(), String("Rendered by the model"));
    EXPECT_FALSE(card->FindChildByTypeAndName<HorizontalLayout>("transparentMode options"));
    EXPECT_FALSE(card->FindChildByType<PipelineColorField>());
    float nativeHeight = card->GetAutoHeight();

    String dir = getenv("O2_PIPELINE_SHOTS") ? String(getenv("O2_PIPELINE_SHOTS")) : String("../../Work/Pipelines/shots");
    Ref<Bitmap> shot;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
    Step();
    ASSERT_TRUE(shot);
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(shot->Save(dir + "/pipeline_native_transparency.png", Bitmap::ImageType::Png));

    auto field = card->FindChildByTypeAndName<Button>("model");
    ASSERT_TRUE(field);
    EXPECT_FLOAT_EQ(field->FindLayer("alpha")->transparency, 1.0f) << "the field marks a model that renders the alpha";
    field->onClick();
    Step();
    auto picker = editor->GetModelPicker();
    ASSERT_TRUE(picker && picker->IsOpen());
    ASSERT_TRUE(picker->FindRow("gemini-3.1-flash-image"));
    picker->Pick("gemini-3.1-flash-image");
    Step(3);

    EXPECT_EQ(gen->GetConfigString("model", ""), String("gemini-3.1-flash-image"));
    EXPECT_EQ(gen->GetConfigString("transparentMode", ""), String("chroma"));
    EXPECT_FLOAT_EQ(gen->GetConfigNumber("chromaTolerance", 0.0f), 45.0f);
    card = editor->GetNodeWidget(gen->id);
    ASSERT_TRUE(card);
    EXPECT_FALSE(card->FindChildByTypeAndName<Label>("native transparency"));
    EXPECT_TRUE(card->FindChildByTypeAndName<HorizontalLayout>("transparentMode options"));
    EXPECT_TRUE(card->FindChildByType<PipelineColorField>());
    EXPECT_GT(card->GetAutoHeight(), nativeHeight + 40.0f) << "the card grows by the rows that came back";
    EXPECT_GE(card->GetCardSize().y, card->GetAutoHeight() - 0.5f);

    // OpenRouter ids follow the same rule: one model of a vendor renders the alpha, another does not
    auto routed = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    routed->SetConfigString("model", "openai/gpt-5-image-mini");
    routed->SetConfigBool("transparentBg", true);
    routed->SetConfigString("transparentMode", "chroma");
    routed->SetConfigBool("paramsOpen", true);
    routed = Open(routed);
    auto routedCard = editor->GetNodeWidget(routed->id);
    ASSERT_TRUE(routedCard);
    EXPECT_TRUE(routedCard->FindChildByTypeAndName<Label>("native transparency"));
    EXPECT_FALSE(routedCard->FindChildByTypeAndName<HorizontalLayout>("transparentMode options"));
    EXPECT_FALSE(routedCard->FindChildByType<PipelineColorField>());

    auto keyed = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    keyed->SetConfigString("model", "openai/gpt-5.4-image-2");
    keyed->SetConfigBool("transparentBg", true);
    keyed->SetConfigString("transparentMode", "chroma");
    keyed->SetConfigBool("paramsOpen", true);
    keyed = Open(keyed);
    auto keyedCard = editor->GetNodeWidget(keyed->id);
    ASSERT_TRUE(keyedCard);
    EXPECT_FALSE(keyedCard->FindChildByTypeAndName<Label>("native transparency"));
    EXPECT_TRUE(keyedCard->FindChildByTypeAndName<HorizontalLayout>("transparentMode options"));
    EXPECT_TRUE(keyedCard->FindChildByType<PipelineColorField>());

    // With the background off there is no note either
    auto plain = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
    plain->SetConfigString("model", "gpt-image-1");
    plain->SetConfigBool("paramsOpen", true);
    plain = Open(plain);
    auto plainCard = editor->GetNodeWidget(plain->id);
    ASSERT_TRUE(plainCard);
    EXPECT_FALSE(plainCard->FindChildByTypeAndName<Label>("native transparency"));
    EXPECT_FALSE(plainCard->FindChildByType<PipelineColorField>());
}

// A resize never leaves the card shorter than its content at the width it ends with: made narrower from its side,
// the drawing tools wrap to more lines and the card's height follows them
TEST_F(PipelineCardSizeUiFixture, ResizeStopsAtTheContentHeightOfTheNewWidth)
{
    auto extract = PipelineNodeRegistry::CreateNode("imageEdit", Vec2F());
    extract = Open(extract);
    auto card = editor->GetNodeWidget(extract->id);
    ASSERT_TRUE(card);

    // A card sized exactly to its content at the default width
    extract->size = Vec2F(card->GetCardWidth(), card->GetAutoHeight());
    card->UpdateFromNode();
    Step(2);
    ASSERT_GT(card->GetAutoHeightForWidth(PipelineNodeWidget::minWidth), extract->size.y + 20.0f);

    RectF rect = card->GetCardRect();
    Vec2F edge = editor->LocalToScreenPoint(Vec2F(rect.right, rect.bottom + 24.0f));
    Drag(edge, edge + Vec2F(-(rect.Width() - PipelineNodeWidget::minWidth) - 20.0f, 0.0f));
    Step(2);

    EXPECT_NEAR(extract->size.x, PipelineNodeWidget::minWidth, 1.0f);
    EXPECT_GE(extract->size.y, card->GetAutoHeightForWidth(extract->size.x) - 0.5f) << "the stored height is shorter than the content";
    EXPECT_NEAR(card->GetCardSize().y, extract->size.y, 0.5f);

    auto body = card->GetBody();
    RectF cardWorld = card->layout->GetWorldRect();
    for (auto& row : body->GetChildWidgets())
        EXPECT_GE(row->layout->GetWorldRect().bottom, cardWorld.bottom - 1.0f) << ((String)row->name).Data();
}
