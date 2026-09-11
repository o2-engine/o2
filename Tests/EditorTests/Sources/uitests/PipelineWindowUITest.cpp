#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/VKCodes.h"
#include "o2/Assets/Assets.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2/Utils/Test/AppTestDriver.h"

#include <chrono>
#include <unistd.h>
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineImport.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineSettingsDlg.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<PipelineNode> AddNode(PipelineGraph& graph, const String& type, const Vec2F& pos)
    {
        auto node = PipelineNodeRegistry::CreateNode(type, pos);
        graph.nodes.Add(node);
        return node;
    }

    void Connect(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& outName, const Ref<PipelineNode>& to, const String& inName)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = from->outputs.Find([&](const PipelinePort& p) { return p.name == outName; })->id;
        edge->toNodeId = to->id;
        edge->toPortId = to->inputs.Find([&](const PipelinePort& p) { return p.name == inName; })->id;
        graph.edges.Add(edge);
    }

    String ScreenshotDir()
    {
        const char* env = getenv("O2_PIPELINE_SHOTS");
        return env ? String(env) : String("../../Work/Pipelines/shots");
    }

    // The test runner's Application draws only the scene: the editor UI root is drawn here by hand so the
    // widgets both render and register for input before every frame is processed
    struct UiDriver
    {
        static void DrawRoot()
        {
            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(30, 31, 34, 255));
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f / 60.0f);
            root->UpdateChildren(1.0f / 60.0f);
            root->UpdateChildrenTransforms();
            root->Draw();
            o2Render.End();
        }

        static void Step(int frames = 1)
        {
            for (int i = 0; i < frames; i++)
            {
                DrawRoot();
                AppTestDriver::PumpFrames(1);
            }
        }

        static void Wait(float seconds)
        {
            int frames = (int)(seconds * 60.0f) + 1;
            Step(frames);
        }

        static Ref<Bitmap> Capture()
        {
            Ref<Bitmap> captured;
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });
            DrawRoot();
            return captured;
        }

        static bool Screenshot(const String& path)
        {
            auto bitmap = Capture();
            if (!bitmap)
                return false;
            o2FileSystem.FolderCreate(o2FileSystem.ExtractPathStr(path), true);
            return bitmap->Save(path, Bitmap::ImageType::Png);
        }

        static void Press(const Vec2F& pos)
        {
            o2Input.OnCursorMoved(pos, 0, false);
            o2Input.OnCursorPressed(pos);
            Step();
        }

        static void Move(const Vec2F& pos, int steps = 10)
        {
            Vec2F start = o2Input.GetCursorPos();
            for (int i = 1; i <= steps; i++)
            {
                o2Input.OnCursorMoved(Math::Lerp(start, pos, (float)i / steps), 0);
                Step();
            }
        }

        static void Release()
        {
            o2Input.OnCursorReleased();
            Step();
        }

        static void Drag(const Vec2F& from, const Vec2F& to, int steps = 12)
        {
            Press(from);
            Move(to, steps);
            Release();
        }
    };

    struct PipelineUiFixture : ::testing::Test
    {
        Ref<PipelineEditor> editor;
        Ref<PipelineAsset> asset;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }
            PipelineUtils::SetWorkPathOverride("../../Work/Pipelines/uitest/");

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
            root->UpdateSelfTransform();
            root->UpdateChildrenTransforms();
            for (int i = 0; i < 12; i++)
            {
                root->Update(0.1f);
                root->UpdateChildren(0.1f);
                root->UpdateChildrenTransforms();
            }
            asset = mmake<PipelineAsset>();
        }

        // Returns the editor's own copy of the node; the editor deserializes the asset document, so test refs go stale on open
        Ref<PipelineNode> Live(const Ref<PipelineNode>& node) const
        {
            return editor->GetGraph()->FindNode(node->id);
        }

        void TearDown() override
        {
            editor = nullptr;
            asset = nullptr;
            PipelineUtils::SetWorkPathOverride("");
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }
    };
}

TEST_F(PipelineUiFixture, BuildsCardsForEveryNodeTypeAndScreenshots)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    text->SetConfigString("text", "pixel art coin for 2D video game. Made of cheese");
    auto prompt = AddNode(graph, "promptGen", Vec2F(330, 0));
    auto gen = AddNode(graph, "nanoBananaGen", Vec2F(660, 0));
    gen->SetConfigBool("transparentBg", true);
    gen->SetConfigString("transparentMode", "chroma");
    auto finish = AddNode(graph, "finishImage", Vec2F(990, 0));
    finish->SetConfigString("assetPath", "Generated/cheesecoin");
    Connect(graph, text, "out", prompt, "description");
    Connect(graph, prompt, "out", gen, "prompt");
    Connect(graph, gen, "out", finish, "in");

    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(5);

    EXPECT_EQ(editor->GetNodeWidget(text->id) != nullptr, true);
    EXPECT_EQ(editor->GetNodeWidget(finish->id) != nullptr, true);

    editor->FitView();
    UiDriver::Wait(0.6f);

    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_simple.png"));

    // Every node type builds a card without crashing
    PipelineGraph all;
    int i = 0;
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        AddNode(all, schema->type, Vec2F((i % 6) * 300.0f, (i / 6) * 700.0f));
        i++;
    }
    all.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    EXPECT_EQ(editor->GetGraph()->nodes.Count(), PipelineNodeRegistry::AllSchemas().Count());
    editor->FitView();
    UiDriver::Wait(0.6f);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_all_nodes.png"));
}

TEST_F(PipelineUiFixture, ConnectsPortsByDragAndUndoes)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    auto edit = AddNode(graph, "textEdit", Vec2F(400, 0));
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.6f);

    text = Live(text);
    edit = Live(edit);
    auto from = editor->GetNodeWidget(text->id);
    auto to = editor->GetNodeWidget(edit->id);
    ASSERT_TRUE(from && to);

    Vec2F fromPort = editor->LocalToScreenPoint(from->GetPortPosition(text->outputs[0].id, false));
    Vec2F toPort = editor->LocalToScreenPoint(to->GetPortPosition(edit->inputs[0].id, true));

    UiDriver::Drag(fromPort, toPort, 16);
    UiDriver::Step(2);

    auto& edges = editor->GetGraph()->edges;
    ASSERT_EQ(edges.Count(), 1);
    EXPECT_EQ(edges[0]->fromNodeId, text->id);
    EXPECT_EQ(edges[0]->toNodeId, edit->id);
    EXPECT_TRUE(asset->IsDirty());

    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_connected.png"));

    // Dragging the card moves the node
    Vec2F header = editor->LocalToScreenPoint(to->GetCardRect().LeftBottom() + Vec2F(60, to->GetCardRect().Height() - 12));
    Vec2F before = edit->position;
    UiDriver::Drag(header, header + Vec2F(80, -40), 16);
    UiDriver::Step(2);
    EXPECT_NE(edit->position, before);
}

TEST_F(PipelineUiFixture, PaintsStrokeAndDragsComposerLayer)
{
    PipelineGraph graph;
    auto draw = AddNode(graph, "drawImage", Vec2F(0, 0));
    auto comp = AddNode(graph, "composer", Vec2F(420, 0));
    Vector<PipelinePort> customs = {
        PipelinePort(PipelineNode::GenerateId(), "sprite", PipelinePortType::Image, true),
        PipelinePort(PipelineNode::GenerateId(), "back", PipelinePortType::Image, true)
    };
    comp->SetCustomInputs(customs);
    PipelineNodeRegistry::SyncNodeWithSchema(comp);
    auto src1 = AddNode(graph, "sourceImage", Vec2F(-300, 0));
    auto src2 = AddNode(graph, "sourceImage", Vec2F(-300, 260));
    Connect(graph, src1, "out", comp, "sprite");
    Connect(graph, src2, "out", comp, "back");

    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    draw = Live(draw);
    comp = Live(comp);

    auto feed = [&](const Ref<PipelineNode>& node, const Ref<Bitmap>& bitmap)
    {
        auto widget = editor->GetNodeWidget(node->id);
        widget->GetRuntime().output = PipelineValue::Image(bitmap);
        widget->OnOutputChanged();
    };
    feed(src1, PipelineImageOps::Blank(64, 64, Color4(255, 0, 0, 255)));
    feed(src2, PipelineImageOps::Blank(128, 96, Color4(0, 0, 255, 255)));
    editor->GetNodeWidget(comp->id)->OnOutputChanged();

    editor->FitView();
    UiDriver::Wait(0.6f);

    // Brush stroke across the draw node canvas stores the drawing
    auto paint = editor->GetNodeWidget(draw->id)->FindChildByType<PipelinePaintEditor>();
    ASSERT_TRUE(paint);
    RectF paintRect = paint->layout->GetWorldRect();
    Vec2F a = editor->LocalToScreenPoint(Vec2F(paintRect.left + paintRect.Width() * 0.3f, paintRect.bottom + paintRect.Height() * 0.4f));
    Vec2F b = editor->LocalToScreenPoint(Vec2F(paintRect.left + paintRect.Width() * 0.7f, paintRect.bottom + paintRect.Height() * 0.4f));
    UiDriver::Drag(a, b, 12);
    UiDriver::Step(2);
    EXPECT_FALSE(draw->GetConfigString("drawing", "").IsEmpty());
    EXPECT_GT(draw->GetConfigNumber("dw", 0), 0.0f);

    // Pressing a composer layer selects it, dragging moves it
    auto stage = editor->GetNodeWidget(comp->id)->FindChildByType<PipelineComposerStage>();
    ASSERT_TRUE(stage);
    auto layers = stage->GetLayers();
    ASSERT_EQ(layers.Count(), 2);
    auto top = layers[1];
    auto placement = stage->GetPlacement(top);
    RectF area = stage->layout->GetWorldRect();
    float vs = stage->GetViewScale();
    float cw = comp->GetConfigNumber("canvasW", 1024), ch = comp->GetConfigNumber("canvasH", 1024);
    Vec2F stageLeftTop(area.Center().x - cw * vs * 0.5f, area.Center().y + ch * vs * 0.5f);
    Vec2F center(stageLeftTop.x + (placement.x + placement.w * 0.5f) * vs, stageLeftTop.y - (placement.y + placement.h * 0.5f) * vs);
    Vec2F from = editor->LocalToScreenPoint(center);
    UiDriver::Drag(from, from + Vec2F(40, 0), 12);
    UiDriver::Step(2);
    EXPECT_EQ(comp->GetConfigString("selectedLayer", ""), top.id);
    auto moved = stage->GetPlacement(top);
    EXPECT_GT(moved.x, placement.x + 1.0f);

    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_paint_composer.png"));
}

namespace
{
    // Screen point (centered, logical units) to a pixel of a captured frame
    Vec2I ScreenToCapture(const Vec2F& screen, const Ref<Bitmap>& capture)
    {
        Vec2I resolution = o2Render.GetResolution();
        float scale = (float)capture->GetSize().x / Math::Max(1, resolution.x);
        return Vec2I((int)((screen.x + resolution.x * 0.5f) * scale), (int)((resolution.y * 0.5f - screen.y) * scale));
    }

    bool IsBackgroundPixel(const Ref<Bitmap>& capture, const Vec2I& p)
    {
        Vec2I size = capture->GetSize();
        if (p.x < 0 || p.y < 0 || p.x >= size.x || p.y >= size.y)
            return false;
        const UInt8* px = PipelineImageOps::Pixel(*capture, p.x, p.y);
        return Math::Abs((int)px[0] - 225) < 14 && Math::Abs((int)px[1] - 232) < 14 && Math::Abs((int)px[2] - 232) < 14;
    }
}

TEST_F(PipelineUiFixture, TextAreasWrapAndStayInsideTheCard)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    String longWord;
    for (int i = 0; i < 120; i++)
        longWord += "w";
    text->SetConfigString("text", longWord);
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.6f);

    auto card = editor->GetNodeWidget(text->id);
    ASSERT_TRUE(card);
    auto edit = card->FindChildByType<EditBox>();
    ASSERT_TRUE(edit);
    EXPECT_TRUE(edit->IsMultiLine());
    EXPECT_TRUE(edit->IsWordWrap());

    // A word that cannot wrap must be clipped at the field, not spill past the card
    auto capture = UiDriver::Capture();
    ASSERT_TRUE(capture);
    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    capture->Save(dir + "/pipeline_clip.png", Bitmap::ImageType::Png);
    RectF editRect = edit->layout->GetWorldRect();
    RectF cardRect = card->GetCardRect();
    for (int i = 1; i <= 5; i++)
    {
        Vec2F probe(cardRect.right + 10.0f + 4.0f * i, editRect.Center().y);
        EXPECT_TRUE(IsBackgroundPixel(capture, ScreenToCapture(editor->LocalToScreenPoint(probe), capture))) << "probe " << i;
    }

    // Spaced text wraps onto several lines
    edit->SetText("pixel art coin for a 2D video game, made of cheese, with a thick dark outline and shiny highlights");
    UiDriver::Step(3);
    EXPECT_GT(edit->GetTextDrawable()->GetRealSize().y, 30.0f);
}

TEST_F(PipelineUiFixture, EditBoxHotkeysUseThePlatformModifier)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    text->SetConfigString("text", "hello");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.3f);

    auto edit = editor->GetNodeWidget(text->id)->FindChildByType<EditBox>();
    ASSERT_TRUE(edit);
    Clipboard::SetText("");
    edit->Focus();
    UiDriver::Step(2);

#if defined PLATFORM_MAC
    KeyboardKey modifier = VK_COMMAND;
#else
    KeyboardKey modifier = VK_CONTROL;
#endif
    o2Input.OnKeyPressed(modifier);
    UiDriver::Step();
    o2Input.OnKeyPressed('A');
    UiDriver::Step();
    o2Input.OnKeyReleased('A');
    o2Input.OnKeyPressed('C');
    UiDriver::Step();
    o2Input.OnKeyReleased('C');
    o2Input.OnKeyReleased(modifier);
    UiDriver::Step(2);

    EXPECT_EQ((String)Clipboard::GetText(), String("hello"));
    EXPECT_EQ((String)edit->GetText(), String("hello"));
}

TEST_F(PipelineUiFixture, PortsDrawAboveFramesAndCardsResizeFromEdges)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    text->SetConfigString("text", "hello");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    text = Live(text);

    auto card = editor->GetNodeWidget(text->id);
    ASSERT_TRUE(card);
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    UiDriver::Step(3);
    card->SetSelected(true);
    UiDriver::Step(8);

    // The selected frame is drawn over the card, the port circle still shows its own color on top of it
    auto capture = UiDriver::Capture();
    ASSERT_TRUE(capture);
    Vec2F portPos = card->GetPortPosition(text->outputs[0].id, false);
    Vec2I px = ScreenToCapture(editor->LocalToScreenPoint(portPos), capture);
    const UInt8* pixel = PipelineImageOps::Pixel(*capture, px.x, px.y);
    Color4 portColor = PipelineEditor::GetPortColor(PipelinePortType::Text);
    EXPECT_NEAR((int)pixel[0], portColor.r, 40);
    EXPECT_NEAR((int)pixel[1], portColor.g, 40);
    EXPECT_NEAR((int)pixel[2], portColor.b, 40);

    // Dragging the left edge moves the node and widens it
    RectF rect = card->GetCardRect();
    Vec2F startPos = text->position;
    Vec2F leftMid(rect.left, rect.bottom + 30.0f);
    Vec2F from = editor->LocalToScreenPoint(leftMid);
    UiDriver::Drag(from, from + Vec2F(-40, 0));
    UiDriver::Step(2);
    EXPECT_NEAR(text->position.x, startPos.x - 40.0f, 2.0f);
    EXPECT_NEAR(text->size.x, rect.Width() + 40.0f, 2.0f);

    // Dragging the bottom edge down makes the card taller
    rect = card->GetCardRect();
    Vec2F bottomMid(rect.Center().x, rect.bottom);
    from = editor->LocalToScreenPoint(bottomMid);
    UiDriver::Drag(from, from + Vec2F(0, -30));
    UiDriver::Step(2);
    EXPECT_NEAR(text->size.y, rect.Height() + 30.0f, 2.0f);

    // Dragging the top edge down moves the node, and the card never gets shorter than its content
    rect = card->GetCardRect();
    float startY = text->position.y;
    Vec2F topMid(rect.Center().x, rect.top);
    from = editor->LocalToScreenPoint(topMid);
    UiDriver::Drag(from, from + Vec2F(0, -200));
    UiDriver::Step(2);
    EXPECT_NEAR(card->GetCardSize().y, card->GetAutoHeight(), 1.0f);
    EXPECT_NEAR(text->position.y, startY + (rect.Height() - card->GetAutoHeight()), 2.0f);

    auto body = card->GetChildWidget("body");
    ASSERT_TRUE(body);
    RectF cardWorld = card->layout->GetWorldRect();
    for (auto& row : body->GetChildWidgets())
    {
        RectF rowRect = row->layout->GetWorldRect();
        EXPECT_GE(rowRect.bottom, cardWorld.bottom - 1.0f);
        EXPECT_LE(rowRect.top, cardWorld.top + 1.0f);
        EXPECT_GE(rowRect.left, cardWorld.left - 1.0f);
        EXPECT_LE(rowRect.right, cardWorld.right + 1.0f);
    }
}

TEST_F(PipelineUiFixture, AudioViewPlaysSeeksAndStopsAtTheEnd)
{
    PushEditorScopeOnStack scope;
    auto view = mmake<PipelineAudioView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(320, PipelineAudioView::height));
    EditorUIRoot.AddWidget(view);
    UiDriver::Step(3);

    // One second of silence, 16-bit mono at 8 kHz
    String pcm;
    pcm.resize(16000, '\0');
    String wav = PipelineAudio::PcmToWav(pcm, 8000, 1);
    view->SetAudio(PipelineValue::Bytes(PipelinePortType::Audio, wav, "audio/wav"), "silence");
    UiDriver::Step(2);
    EXPECT_NEAR(view->GetDuration(), 1.0f, 0.1f);
    EXPECT_FALSE(view->IsPlaying());

    auto play = view->GetControls()->GetPlayButton();
    ASSERT_TRUE(play);
    Vec2F center = play->layout->GetWorldRect().Center();
    UiDriver::Press(center);
    UiDriver::Release();
    UiDriver::Step(2);
    EXPECT_TRUE(view->IsPlaying());

    UiDriver::Wait(0.3f);
    EXPECT_GT(view->GetTime(), 0.15f);

    view->Seek(0.5f);
    EXPECT_NEAR(view->GetTime(), 0.5f, 0.05f);

    UiDriver::Wait(0.8f);
    EXPECT_FALSE(view->IsPlaying());
    EXPECT_NEAR(view->GetTime(), 1.0f, 0.1f);

    view->GetControls()->SetLoop(true);
    view->Play();
    UiDriver::Wait(1.3f);
    EXPECT_TRUE(view->IsPlaying());
    view->Pause();
    EXPECT_FALSE(view->IsPlaying());
}

// The Save to Assets button runs the finish node, writes the file and rebuilds the assets; the assets window
// reacts to the rebuild the same way it does in the editor
TEST_F(PipelineUiFixture, FinishNodeSaveToAssetsRebuildsAssetsAndKeepsRunning)
{
    PushEditorScopeOnStack scope;
    if (!AssetsWindow::IsSingletonInitialzed())
        mmake<AssetsWindow>();

    // The rebuild runs the tool from the working directory; link the built one in when it is missing
    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    auto red = PipelineImageOps::Blank(32, 32, Color4(255, 0, 0, 255));
    String uploadsDir = PipelineUtils::GetUploadsPath();
    o2FileSystem.FolderCreate(uploadsDir, true);
    ASSERT_TRUE(red->Save(uploadsDir + "finish_source.png", Bitmap::ImageType::Png));

    PipelineGraph graph;
    auto source = AddNode(graph, "sourceImage", Vec2F(0, 0));
    source->SetConfigString("uploadId", "finish_source.png");
    auto finish = AddNode(graph, "finishImage", Vec2F(330, 0));
    finish->SetConfigString("assetPath", "Generated/uitest/finish_test");
    Connect(graph, source, "out", finish, "in");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.5f);

    auto card = editor->GetNodeWidget(finish->id);
    ASSERT_TRUE(card);
    auto save = card->FindChildByTypeAndName<Button>("save");
    ASSERT_TRUE(save);
    Vec2F center = editor->LocalToScreenPoint(save->layout->GetWorldRect().Center());
    UiDriver::Press(center);
    UiDriver::Release();
    UiDriver::Step(2);

    // A cached branch can finish within the click frame, so the run is only waited for, never asserted
    for (int i = 0; i < 3600 && editor->IsRunning(); i++)
        UiDriver::Step();
    EXPECT_FALSE(editor->IsRunning());

    String written = o2Assets.GetAssetsPath() + "Generated/uitest/finish_test.png";
    EXPECT_TRUE(o2FileSystem.IsFileExist(written));
    ASSERT_TRUE(editor->GetNodeWidget(finish->id));
    EXPECT_EQ(editor->GetNodeWidget(finish->id)->GetRuntime().state, String("idle"));
    EXPECT_TRUE(editor->GetNodeWidget(finish->id)->GetRuntime().fresh);
    UiDriver::Step(3);

    o2FileSystem.FolderRemove(o2Assets.GetAssetsPath() + "Generated/uitest", true);
    o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + "Generated/uitest.meta");
}

// Every control of every node type stays inside its card at the default card width
TEST_F(PipelineUiFixture, FinishFolderMenuListsFoldersWithoutSubmenus)
{
    PipelineGraph graph;
    auto finish = AddNode(graph, "finishImage", Vec2F());
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.5f);

    auto card = editor->GetNodeWidget(Live(finish)->id);
    ASSERT_TRUE(card);
    auto browse = card->FindChildByTypeAndName<Button>("browse");
    ASSERT_TRUE(browse);
    Vec2F center = editor->LocalToScreenPoint(browse->layout->GetWorldRect().Center());
    UiDriver::Press(center);
    UiDriver::Release();
    UiDriver::Step(2);

    auto menu = editor->GetPopupMenu();
    ASSERT_TRUE(menu);
    EXPECT_TRUE(menu->IsEnabled());
    auto& items = menu->GetItems();
    ASSERT_GE(items.Count(), 1);
    EXPECT_EQ(items[0]->text, WString("Assets"));
    for (auto& item : items)
    {
        EXPECT_TRUE(item->subItems.IsEmpty()) << (String)item->text;
        EXPECT_FALSE(((String)item->text).Contains("/")) << (String)item->text;
    }

    if (items.Count() > 1)
    {
        items[1]->onClick();
        UiDriver::Step(2);
        String folder = ((String)items[1]->text).ReplacedAll(" > ", "/");
        EXPECT_TRUE(Live(finish)->GetConfigString("assetPath", "").StartsWith(folder + "/")) << folder;
    }
}

TEST_F(PipelineUiFixture, LinksFollowTheZoomButStayAtLeastOnePixel)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F(0, 0));
    auto edit = AddNode(graph, "textEdit", Vec2F(700, 0));
    Connect(graph, text, "out", edit, "text");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    text = Live(text);
    edit = Live(edit);

    Vec2F from = editor->GetNodeWidget(text->id)->GetPortPosition(text->outputs[0].id, false);
    auto input = edit->inputs.Find([](const PipelinePort& p) { return p.name == "text"; });
    ASSERT_TRUE(input);
    Vec2F to = editor->GetNodeWidget(edit->id)->GetPortPosition(input->id, true);
    ASSERT_NEAR(from.y, to.y, 0.5f);
    Vec2F middle = (from + to) * 0.5f;

    // Counts the rows around the link middle painted in the link color: the on-screen thickness
    auto paintedRows = [&](float scale)
    {
        editor->SetView(middle, scale);
        UiDriver::Step(4);
        auto capture = UiDriver::Capture();
        Vec2I px = ScreenToCapture(editor->LocalToScreenPoint(middle), capture);
        int rows = 0;
        for (int dy = -6; dy <= 6; dy++)
        {
            const UInt8* p = PipelineImageOps::Pixel(*capture, px.x, px.y + dy);
            if ((int)p[2] - (int)p[0] > 40)
                rows++;
        }
        return rows;
    };

    int far = paintedRows(12.0f);
    int normal = paintedRows(1.0f);
    int close = paintedRows(0.5f);
    EXPECT_GE(far, 1);
    EXPECT_GE(normal, 1);
    EXPECT_LE(normal, 3);
    EXPECT_GT(close, normal);
}

TEST_F(PipelineUiFixture, NodeControlsStayInsideCardsAtDefaultWidth)
{
    PipelineGraph graph;
    int i = 0;
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        auto node = AddNode(graph, schema->type, Vec2F((i % 6) * 320.0f, (i / 6) * 800.0f));
        if (schema->type == "imageEdit" || schema->type == "imageExtract")
            node->SetConfigBool("drawOver", true);
        i++;
    }
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.6f);

    Function<void(const Ref<Widget>&, const RectF&, const String&)> check;
    check = [&](const Ref<Widget>& widget, const RectF& card, const String& type)
    {
        for (auto& child : widget->GetChildWidgets())
        {
            if (!child->IsEnabled())
                continue;

            RectF rect = child->layout->GetWorldRect();
            if (rect.Width() > 0.0f && rect.Height() > 0.0f)
            {
                String where = type + " / " + child->GetName();
                EXPECT_LE(rect.right, card.right + 1.0f) << where;
                EXPECT_GE(rect.left, card.left - 1.0f) << where;
                EXPECT_GE(rect.bottom, card.bottom - 1.0f) << where;
            }
            check(child, card, type);
        }
    };

    for (auto& node : editor->GetGraph()->nodes)
    {
        auto widget = editor->GetNodeWidget(node->id);
        ASSERT_TRUE(widget) << node->nodeType;
        auto body = widget->GetChildWidget("body");
        ASSERT_TRUE(body) << node->nodeType;
        check(body, widget->layout->GetWorldRect(), node->nodeType);
    }

    Ref<PipelineNode> draw, finish;
    for (auto& node : editor->GetGraph()->nodes)
    {
        if (node->nodeType == "drawImage") draw = node;
        if (node->nodeType == "finishImage") finish = node;
    }
    ASSERT_TRUE(draw && finish);
    RectF drawRect = editor->GetNodeWidget(draw->id)->GetCardRect();
    RectF finishRect = editor->GetNodeWidget(finish->id)->GetCardRect();
    RectF both(Math::Min(drawRect.left, finishRect.left), Math::Max(drawRect.top, finishRect.top),
               Math::Max(drawRect.right, finishRect.right), Math::Min(drawRect.bottom, finishRect.bottom));
    editor->SetView(both.Center(), 1.0f);
    UiDriver::Step(4);
    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_wrap_finish.png"));
}

// Every node type has an icon asset, and the add-node menu opens with them
TEST_F(PipelineUiFixture, AddNodeMenuCarriesNodeIcons)
{
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        String icon = PipelineNodeWidget::IconForType(schema->type);
        EXPECT_TRUE(o2Assets.IsAssetExist(icon)) << schema->type << " -> " << icon;
    }

    PipelineGraph graph;
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    // A right click on empty canvas fills and opens the add-node menu
    Vec2F at = editor->LocalToScreenPoint(Vec2F(-200, 150));
    o2Input.OnCursorMoved(at, 0, false);
    UiDriver::Step();
    o2Input.OnAltCursorPressed(at);
    UiDriver::Step();
    o2Input.OnAltCursorReleased();
    UiDriver::Wait(0.5f);
    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_add_menu.png"));
    EXPECT_TRUE(editor->GetContextMenu()->IsEnabled());
    editor->GetContextMenu()->Hide();
    UiDriver::Step(2);
}

// Screenshots of the Gemini showcase asset with its cached results (PIPELINE_SHOWCASE=1)
TEST_F(PipelineUiFixture, GeminiShowcaseScreenshots)
{
    if (!getenv("PIPELINE_SHOWCASE"))
        GTEST_SKIP() << "PIPELINE_SHOWCASE is not set";

    PipelineUtils::SetWorkPathOverride("");
    const String assetPath = "Pipelines/GeminiShowcase.pipeline";
    ASSERT_TRUE(o2Assets.IsAssetExist(assetPath));
    AssetRef<PipelineAsset> showcase(assetPath);
    asset = showcase;
    ASSERT_TRUE(asset);
    ASSERT_TRUE(asset->document.FindMember("graph") != nullptr);

    editor->SetAsset(asset);
    ASSERT_GT(editor->GetGraph()->nodes.Count(), 10);
    UiDriver::Step(5);
    editor->FitView();
    UiDriver::Wait(0.7f);

    // Video previews are extracted in the background; wait for them so the players show a frame
    for (int frame = 0; frame < 900; frame++)
    {
        bool pending = false;
        for (auto& node : editor->GetGraph()->nodes)
        {
            auto widget = editor->GetNodeWidget(node->id);
            auto video = widget ? widget->FindChildByType<PipelineVideoView>() : nullptr;
            if (video && video->HasVideo() && !video->IsReady())
                pending = true;
        }
        if (!pending)
            break;

        UiDriver::Step();
    }

    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/showcase_all.png"));

    struct View { const char* name; Vec2F center; float scale; };
    // Canvas y grows upwards: a node at graph y = -900 sits at canvas y = +900. Scale < 1 zooms in
    View views[] = {
        { "showcase_image_chain", Vec2F(990, -370), 1.05f },
        { "showcase_extract_video", Vec2F(1100, -1380), 0.95f },
        { "showcase_text_speech", Vec2F(880, 620), 0.8f },
        { "showcase_music", Vec2F(700, 1330), 0.65f },
    };
    for (auto& view : views)
    {
        editor->SetView(view.center, view.scale);
        UiDriver::Step(4);
        EXPECT_TRUE(UiDriver::Screenshot(dir + "/" + view.name + ".png")) << view.name;
    }
}

TEST_F(PipelineUiFixture, CacheIdFollowsTheGraphAcrossAssets)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(2);

    // An asset saved before the id existed adopts its UID, so results cached under it stay reachable
    String uid = (String)asset->GetUID();
    ASSERT_FALSE(uid.IsEmpty());
    EXPECT_EQ(editor->GetPipelineId(), uid);
    EXPECT_EQ(editor->GetGraph()->id, uid);

    PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(uid, text->id, "txt"), "cached");

    // The same graph saved into another asset (save as, duplicate) keeps its cache key and its results
    auto copy = mmake<PipelineAsset>();
    editor->GetGraph()->SaveToAsset(*copy);
    editor->SetAsset(copy);
    UiDriver::Step(2);
    EXPECT_NE((String)copy->GetUID(), uid);
    EXPECT_EQ(editor->GetPipelineId(), uid);
    auto runtime = editor->GetRuntime(text->id);
    ASSERT_TRUE(runtime);
    EXPECT_EQ(runtime->output.data, String("cached"));

    o2FileSystem.FolderRemove(PipelineExecutor::GetCachePath(uid), true);
}

TEST_F(PipelineUiFixture, CullsOffscreenCardsAndDropsDetailsWhenZoomedOut)
{
    PipelineGraph graph;
    Vector<Ref<PipelineNode>> nodes;
    for (int i = 0; i < 40; i++)
        nodes.Add(AddNode(graph, "sourceText", Vec2F((i % 8) * 2000.0f, (i / 8) * 2000.0f)));
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    // Close up only the cards around the first one are inside the view
    auto first = editor->GetNodeWidget(nodes[0]->id);
    ASSERT_TRUE(first);
    editor->SetView(first->GetCardRect().Center(), 1.0f);
    UiDriver::Step(3);
    int visible = 0;
    for (auto& node : nodes)
    {
        if (!editor->GetNodeWidget(node->id)->IsCulled())
            visible++;
    }
    EXPECT_GE(visible, 1);
    EXPECT_LT(visible, 8);
    EXPECT_FALSE(first->IsCulled());
    EXPECT_TRUE(first->IsDetailed());
    EXPECT_TRUE(editor->GetNodeWidget(nodes[39]->id)->IsCulled());

    // Fitting a graph this wide needs more zoom-out than the old limit of 6 allowed
    editor->FitView();
    UiDriver::Wait(0.8f);
    EXPECT_GT(editor->GetCamera().GetScale2D().x, 6.0f);
    for (auto& node : nodes)
    {
        auto card = editor->GetNodeWidget(node->id);
        EXPECT_FALSE(card->IsCulled());
        EXPECT_FALSE(card->IsDetailed());
    }

    auto start = std::chrono::steady_clock::now();
    UiDriver::Step(20);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 20.0;
    printf("[perf] 40 cards fitted: %.2f ms per frame\n", ms);
}

TEST_F(PipelineUiFixture, FarZoomHidesControlsButKeepsContentInPlace)
{
    // A provider node: it keeps its play button, unlike self-applying effect nodes
    PipelineGraph graph;
    auto image = AddNode(graph, "nanoBananaGen", Vec2F(0, 0));
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    auto card = editor->GetNodeWidget(Live(image)->id);
    ASSERT_TRUE(card);
    card->GetRuntime().output = PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(255, 0, 0, 255)));
    card->OnOutputChanged();
    auto view = card->FindChildByType<PipelineImageView>();
    auto play = card->FindChildByTypeAndName<Button>("play");
    ASSERT_TRUE(view && play);

    auto pixelAt = [&](const Ref<Bitmap>& capture, const Vec2F& canvasPoint)
    {
        Vec2I px = ScreenToCapture(editor->LocalToScreenPoint(canvasPoint), capture);
        const UInt8* p = PipelineImageOps::Pixel(*capture, px.x, px.y);
        return Color4(p[0], p[1], p[2], 255);
    };
    auto distance = [](const Color4& a, const Color4& b) { return Math::Abs(a.r - b.r) + Math::Abs(a.g - b.g) + Math::Abs(a.b - b.b); };

    for (float scale : { 1.0f, 6.0f })
    {
        editor->SetView(card->GetCardRect().Center(), scale);
        UiDriver::Step(4);
        EXPECT_EQ(card->IsDetailed(), scale < 2.0f) << "scale " << scale;
        auto capture = UiDriver::Capture();
        ASSERT_TRUE(capture);

        // What the node produced stays drawn where it is
        Color4 imagePixel = pixelAt(capture, view->layout->GetWorldRect().Center());
        EXPECT_GT(imagePixel.r, 200) << "scale " << scale;
        EXPECT_LT(imagePixel.g, 80) << "scale " << scale;

        // The play button is a control: drawn close up, gone from afar so its pixel matches the header band
        Vec2F playCenter = play->layout->GetWorldRect().Center();
        int diff = distance(pixelAt(capture, playCenter), pixelAt(capture, playCenter - Vec2F(30.0f, 0.0f)));
        if (scale < 2.0f)
            EXPECT_GT(diff, 60);
        else
            EXPECT_LT(diff, 30);
    }
}

TEST_F(PipelineUiFixture, DrawToolbarSlidersKeepAUsableTrack)
{
    PipelineGraph graph;
    auto draw = AddNode(graph, "drawImage", Vec2F());
    auto edit = AddNode(graph, "imageEdit", Vec2F(400, 0));
    edit->SetConfigBool("drawOver", true);
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.6f);

    for (auto& node : editor->GetGraph()->nodes)
    {
        auto card = editor->GetNodeWidget(node->id);
        ASSERT_TRUE(card);
        auto size = card->FindChildByTypeAndName<PipelineSlider>("size");
        auto opacity = card->FindChildByTypeAndName<PipelineSlider>("opacity");
        ASSERT_TRUE(size && opacity) << node->nodeType;
        EXPECT_GE(size->GetTrackWidth(), 60.0f) << node->nodeType;
        EXPECT_GE(opacity->GetTrackWidth(), 60.0f) << node->nodeType;

        RectF cardRect = card->layout->GetWorldRect();
        EXPECT_LE(size->layout->GetWorldRect().right, cardRect.right + 1.0f) << node->nodeType;
        EXPECT_LE(opacity->layout->GetWorldRect().right, cardRect.right + 1.0f) << node->nodeType;

        // Every tool sits inside the toolbar band, so none is hidden under the stage
        auto toolbar = card->FindChildByTypeAndName<PipelineWrapRow>("toolbar");
        ASSERT_TRUE(toolbar) << node->nodeType;
        RectF band = toolbar->layout->GetWorldRect();
        for (auto& tool : toolbar->GetChildWidgets())
        {
            if (!tool->IsEnabled())
                continue;

            RectF rect = tool->layout->GetWorldRect();
            EXPECT_GE(rect.bottom, band.bottom - 1.0f) << node->nodeType << " / " << tool->GetName();
            EXPECT_LE(rect.right, band.right + 1.0f) << node->nodeType << " / " << tool->GetName();
        }
    }

    // The palette wraps inside the card too
    auto card = editor->GetNodeWidget(Live(draw)->id);
    auto color = card->FindChildByTypeAndName<Button>("color");
    ASSERT_TRUE(color);
    Vec2F at = editor->LocalToScreenPoint(color->layout->GetWorldRect().Center());
    UiDriver::Press(at);
    UiDriver::Release();
    UiDriver::Step(3);
    auto palette = card->FindChildByTypeAndName<PipelineWrapRow>("palette");
    ASSERT_TRUE(palette);
    EXPECT_TRUE(palette->IsEnabled());
    RectF cardRect = card->layout->GetWorldRect();
    for (auto& swatch : palette->GetChildWidgets())
    {
        EXPECT_LE(swatch->layout->GetWorldRect().right, cardRect.right + 1.0f);
        EXPECT_GE(swatch->layout->GetWorldRect().left, cardRect.left - 1.0f);
    }
    EXPECT_TRUE(UiDriver::Screenshot(ScreenshotDir() + "/pipeline_draw_toolbar.png"));
}

TEST_F(PipelineUiFixture, StoredDrawingKeepsItsResolution)
{
    auto bitmap = PipelineImageOps::Blank(64, 48, Color4(0, 0, 255, 255));
    String dataUrl = PipelineUtils::BytesToDataUrl(PipelineValue::Image(bitmap).GetPngBytes(), "image/png");
    PipelineGraph graph;
    auto draw = AddNode(graph, "drawImage", Vec2F());
    draw->SetConfigString("drawing", dataUrl);
    draw->SetConfigNumber("dw", 64);
    draw->SetConfigNumber("dh", 48);
    auto blank = AddNode(graph, "drawImage", Vec2F(400, 0));
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->FitView();
    UiDriver::Wait(0.6f);

    // An imported drawing is edited at its own size instead of being squeezed into the stage
    auto paint = editor->GetNodeWidget(Live(draw)->id)->FindChildByType<PipelinePaintEditor>();
    ASSERT_TRUE(paint);
    EXPECT_EQ(paint->GetResolution(), Vec2I(64, 48));

    // A blank canvas takes twice the stage size, so new strokes stay crisp when zoomed in
    auto blankPaint = editor->GetNodeWidget(Live(blank)->id)->FindChildByType<PipelinePaintEditor>();
    ASSERT_TRUE(blankPaint);
    Vec2I res = blankPaint->GetResolution();
    EXPECT_GT(res.x, 300);
    EXPECT_GT(res.y, 200);
}

// Loads a real pipeline file (O2_PIPELINE_BIG_ASSET=<path>), fits it, screenshots it and reports the frame cost
TEST_F(PipelineUiFixture, LargeAssetFitsAndStaysFast)
{
    const char* path = getenv("O2_PIPELINE_BIG_ASSET");
    if (!path)
        GTEST_SKIP() << "set O2_PIPELINE_BIG_ASSET to a .pipeline file";

    ASSERT_TRUE(asset->document.LoadFromFile(path)) << path;
    editor->SetAsset(asset);
    UiDriver::Step(3);
    int count = editor->GetGraph()->nodes.Count();
    editor->FitView();
    UiDriver::Wait(1.0f);

    auto timeFrames = [](int frames)
    {
        auto start = std::chrono::steady_clock::now();
        UiDriver::Step(frames);
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / frames;
    };

    int culled = 0, detailed = 0;
    for (auto& node : editor->GetGraph()->nodes)
    {
        auto card = editor->GetNodeWidget(node->id);
        if (card->IsCulled()) culled++;
        if (card->IsDetailed()) detailed++;
    }
    printf("[perf] %d nodes fitted at scale %.1f: %.2f ms per frame, culled %d, detailed %d\n",
           count, editor->GetCamera().GetScale2D().x, timeFrames(30), culled, detailed);
    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_big_fit.png"));

    editor->SetView(editor->GetGraph()->nodes[0]->position * Vec2F(1, -1), 1.0f);
    UiDriver::Wait(0.5f);
    culled = 0;
    for (auto& node : editor->GetGraph()->nodes)
    {
        if (editor->GetNodeWidget(node->id)->IsCulled())
            culled++;
    }
    printf("[perf] %d nodes at scale 1: %.2f ms per frame, culled %d\n", count, timeFrames(30), culled);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_big_close.png"));
}

// Imports a real AssetsLine export (O2_PIPELINE_IMPORT_FILE=<zip or json>) exactly the way the import button does:
// asset file, assets rebuild, load by path, cards, previews and video extraction
TEST_F(PipelineUiFixture, ImportsRealExportWithoutThrowing)
{
    const char* path = getenv("O2_PIPELINE_IMPORT_FILE");
    if (!path)
        GTEST_SKIP() << "set O2_PIPELINE_IMPORT_FILE to an AssetsLine export";

    if (!AssetsWindow::IsSingletonInitialzed())
        mmake<AssetsWindow>();

    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    String error;
    String assetPath = PipelineImport::ImportFile(path, "Pipelines/uitest-import/", error);
    ASSERT_FALSE(assetPath.IsEmpty()) << error;
    printf("[import] asset %s\n", assetPath.Data());

    AssetRef<PipelineAsset> imported(assetPath);
    ASSERT_TRUE(imported);
    editor->SetAsset(imported);
    UiDriver::Step(5);
    editor->FitView();
    UiDriver::Wait(0.6f);
    printf("[import] %d nodes shown, cache id %s\n", editor->GetGraph()->nodes.Count(), editor->GetPipelineId().Data());

    // Video previews are extracted in the background; the crash may hide in their arrival
    for (int i = 0; i < 60 * 40; i++)
    {
        bool pending = false;
        for (auto& node : editor->GetGraph()->nodes)
        {
            auto widget = editor->GetNodeWidget(node->id);
            auto video = widget ? widget->FindChildByType<PipelineVideoView>() : nullptr;
            if (video && video->HasVideo() && !video->IsReady())
                pending = true;
        }
        if (!pending)
            break;
        UiDriver::Step();
    }
    UiDriver::Step(10);

    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_import_real.png"));

    String cacheId = editor->GetPipelineId();
    editor->SetAsset(nullptr);
    UiDriver::Step(2);
    o2FileSystem.FolderRemove(PipelineExecutor::GetCachePath(cacheId), true);
    o2FileSystem.FolderRemove(o2Assets.GetAssetsPath() + "Pipelines/uitest-import", true);
    o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + "Pipelines/uitest-import.meta");
    o2Assets.RebuildAssets();
}

TEST_F(PipelineUiFixture, RunAllRunsEveryFinishNode)
{
    if (!AssetsWindow::IsSingletonInitialzed())
        mmake<AssetsWindow>();
    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    text->SetConfigString("text", "hello");
    auto first = AddNode(graph, "finishText", Vec2F(400, 0));
    first->SetConfigString("assetPath", "Generated/uitest-runall/first");
    auto second = AddNode(graph, "finishText", Vec2F(400, 300));
    second->SetConfigString("assetPath", "Generated/uitest-runall/second");
    Connect(graph, text, "out", first, "in");
    Connect(graph, text, "out", second, "in");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    first = Live(first);
    second = Live(second);

    // The second target waits in the queue and starts from the frame loop once the first run is done
    editor->RunAll();
    for (int i = 0; i < 1800 && (editor->IsRunning() || !editor->GetNodeWidget(second->id)->GetRuntime().fresh); i++)
        UiDriver::Step();

    String dir = o2Assets.GetAssetsPath() + "Generated/uitest-runall/";
    EXPECT_TRUE(o2FileSystem.IsFileExist(dir + "first.txt"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(dir + "second.txt"));
    EXPECT_TRUE(editor->GetNodeWidget(first->id)->GetRuntime().fresh);
    EXPECT_TRUE(editor->GetNodeWidget(second->id)->GetRuntime().fresh);
    UiDriver::Step(3);

    o2FileSystem.FolderRemove(dir, true);
    o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + "Generated/uitest-runall.meta");
    o2Assets.RebuildAssets();
}

TEST_F(PipelineUiFixture, WheelOverACardZoomsTheCanvas)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    text->SetConfigString("text", "some text to hover");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    auto card = editor->GetNodeWidget(Live(text)->id);
    ASSERT_TRUE(card);
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    UiDriver::Step(3);
    auto area = card->FindChildByType<EditBox>();
    ASSERT_TRUE(area);

    // The text area is a scrollable control inside the card layer; the wheel over it still zooms the canvas
    o2Input.OnCursorMoved(editor->LocalToScreenPoint(area->layout->GetWorldRect().Center()), 0, false);
    UiDriver::Step(2);
    float before = editor->GetCamera().GetScale2D().x;
    o2Input.OnMouseWheel(-120.0f);
    UiDriver::Step(8);
    EXPECT_GT(Math::Abs(editor->GetCamera().GetScale2D().x - before), 0.01f);
}

TEST_F(PipelineUiFixture, LocalNodeReappliesOverTheLastProviderResult)
{
    PipelineGraph graph;
    auto gen = AddNode(graph, "nanoBananaGen", Vec2F());
    auto outline = AddNode(graph, "imageOutline", Vec2F(400, 0));
    Connect(graph, gen, "out", outline, "image");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    gen = Live(gen);
    outline = Live(outline);

    // The provider's result exists only as its preview, the way an imported or re-edited pipeline has it
    auto bitmap = PipelineImageOps::Blank(16, 16, Color4(255, 0, 0, 255));
    auto genWidget = editor->GetNodeWidget(gen->id);
    genWidget->GetRuntime().output = PipelineValue::Image(bitmap);
    genWidget->OnOutputChanged();
    PipelineUtils::WriteFileBytes(PipelineExecutor::GetPreviewPath(editor->GetPipelineId(), gen->id, "png"), PipelineValue::Image(bitmap).GetPngBytes());

    outline->SetConfigNumber("width", 6);
    editor->OnNodeConfigChanged(editor->GetNodeWidget(outline->id), "width", true);
    for (int i = 0; i < 600 && !editor->GetNodeWidget(outline->id)->GetRuntime().output.IsValid(); i++)
        UiDriver::Step();

    auto& runtime = editor->GetNodeWidget(outline->id)->GetRuntime();
    EXPECT_TRUE(runtime.output.IsImage());
    EXPECT_NE(runtime.state, String("error"));
    o2FileSystem.FolderRemove(PipelineExecutor::GetCachePath(editor->GetPipelineId()), true);
}

TEST_F(PipelineUiFixture, SelectionOutlineFollowsTheZoomButStaysVisible)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    auto card = editor->GetNodeWidget(Live(text)->id);
    ASSERT_TRUE(card);
    card->SetSelected(true);

    RectF rect = card->layout->GetWorldRect();
    Vec2F edge(rect.left - 2.0f, rect.Center().y);
    auto tealNear = [&](float scale)
    {
        editor->SetView(card->GetCardRect().Center(), scale);
        UiDriver::Step(4);
        auto capture = UiDriver::Capture();
        Vec2I px = ScreenToCapture(editor->LocalToScreenPoint(edge), capture);
        for (int dx = -2; dx <= 2; dx++)
        {
            const UInt8* p = PipelineImageOps::Pixel(*capture, px.x + dx, px.y);
            if (p[0] < 70 && p[1] > 120 && p[2] > 100 && p[2] < p[1] + 20)
                return true;
        }
        return false;
    };
    EXPECT_TRUE(tealNear(1.0f));
    EXPECT_TRUE(tealNear(12.0f));
}

TEST_F(PipelineUiFixture, ComposerSavesLayersAsAssets)
{
    if (!AssetsWindow::IsSingletonInitialzed())
        mmake<AssetsWindow>();
    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    PipelineGraph graph;
    auto comp = AddNode(graph, "composer", Vec2F(420, 0));
    Vector<PipelinePort> customs = {
        PipelinePort(PipelineNode::GenerateId(), "sprite", PipelinePortType::Image, true),
        PipelinePort(PipelineNode::GenerateId(), "back", PipelinePortType::Image, true)
    };
    comp->SetCustomInputs(customs);
    PipelineNodeRegistry::SyncNodeWithSchema(comp);
    comp->SetConfigString("layersFolder", "Generated/uitest-composer");
    comp->SetConfigString("layersName", "part");
    auto src1 = AddNode(graph, "sourceImage", Vec2F(-300, 0));
    auto src2 = AddNode(graph, "sourceImage", Vec2F(-300, 260));
    Connect(graph, src1, "out", comp, "sprite");
    Connect(graph, src2, "out", comp, "back");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    comp = Live(comp);

    auto feed = [&](const Ref<PipelineNode>& node, const Ref<Bitmap>& bitmap)
    {
        auto widget = editor->GetNodeWidget(node->id);
        widget->GetRuntime().output = PipelineValue::Image(bitmap);
        widget->OnOutputChanged();
    };
    feed(Live(src1), PipelineImageOps::Blank(64, 64, Color4(255, 0, 0, 255)));
    feed(Live(src2), PipelineImageOps::Blank(128, 96, Color4(0, 0, 255, 255)));
    auto card = editor->GetNodeWidget(comp->id);
    card->OnOutputChanged();
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    UiDriver::Step(4);

    auto save = card->FindChildByTypeAndName<Button>("save layers");
    ASSERT_TRUE(save);
    String dir0 = ScreenshotDir();
    o2FileSystem.FolderCreate(dir0, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir0 + "/pipeline_composer_assets.png"));
    UiDriver::Press(editor->LocalToScreenPoint(save->layout->GetWorldRect().Center()));
    UiDriver::Release();
    UiDriver::Step(3);

    String dir = o2Assets.GetAssetsPath() + "Generated/uitest-composer/";
    EXPECT_TRUE(o2FileSystem.IsFileExist(dir + "part_sprite.png"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(dir + "part_back.png"));

    o2FileSystem.FolderRemove(dir, true);
    o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + "Generated/uitest-composer.meta");
    o2Assets.RebuildAssets();
}

// Editing an early node and running everything must reach every branch end, not just the finish nodes
TEST_F(PipelineUiFixture, RunAllReachesBranchEndsWithoutFinishNodes)
{
    PipelineGraph graph;
    auto source = AddNode(graph, "sourceText", Vec2F());
    source->SetConfigString("text", "hello");
    auto shallow = AddNode(graph, "textCompose", Vec2F(400, 0));
    auto deepA = AddNode(graph, "textCompose", Vec2F(400, 300));
    auto deepB = AddNode(graph, "textCompose", Vec2F(800, 300));
    Connect(graph, source, "out", shallow, "template");
    Connect(graph, source, "out", deepA, "template");
    Connect(graph, deepA, "out", deepB, "template");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    int actorsBefore = o2Scene.GetRootActors().Count();
    int editablesBefore = o2Scene.GetAllEditableObjects().Count();

    editor->RunAll();
    for (int i = 0; i < 1800; i++)
    {
        bool allFresh = true;
        for (auto& node : editor->GetGraph()->nodes)
        {
            if (node->nodeType == "textCompose" && !editor->GetNodeWidget(node->id)->GetRuntime().fresh)
                allFresh = false;
        }
        if (allFresh && !editor->IsRunning())
            break;

        UiDriver::Step();
    }

    for (auto& node : editor->GetGraph()->nodes)
    {
        if (node->nodeType != "textCompose")
            continue;

        auto& runtime = editor->GetNodeWidget(node->id)->GetRuntime();
        EXPECT_TRUE(runtime.fresh) << node->id;
        EXPECT_EQ(runtime.output.data, String("hello"));
    }

    // The cards refreshed by executor events are editor UI, not scene content
    EXPECT_EQ(o2Scene.GetRootActors().Count(), actorsBefore);
    EXPECT_EQ(o2Scene.GetAllEditableObjects().Count(), editablesBefore);
    o2FileSystem.FolderRemove(PipelineExecutor::GetCachePath(editor->GetPipelineId()), true);
}

// Pipeline cards are editor UI: rebuilding one must not register widgets in the scene
TEST_F(PipelineUiFixture, CardRebuildsOutsideEditorScopeStayOutOfTheScene)
{
    PipelineGraph graph;
    auto comp = AddNode(graph, "composer", Vec2F());
    Vector<PipelinePort> customs = {
        PipelinePort(PipelineNode::GenerateId(), "sprite", PipelinePortType::Image, true),
        PipelinePort(PipelineNode::GenerateId(), "back", PipelinePortType::Image, true)
    };
    comp->SetCustomInputs(customs);
    PipelineNodeRegistry::SyncNodeWithSchema(comp);
    auto src = AddNode(graph, "sourceImage", Vec2F(-300, 0));
    Connect(graph, src, "out", comp, "sprite");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    auto card = editor->GetNodeWidget(Live(comp)->id);
    ASSERT_TRUE(card);
    int actorsBefore = o2Scene.GetRootActors().Count();
    int editablesBefore = o2Scene.GetAllEditableObjects().Count();

    // Straight from the test body, like an executor event arriving between frames
    auto feed = editor->GetNodeWidget(Live(src)->id);
    feed->GetRuntime().output = PipelineValue::Image(PipelineImageOps::Blank(8, 8, Color4(255, 0, 0, 255)));
    feed->OnOutputChanged();
    for (int i = 0; i < 5; i++)
    {
        card->OnOutputChanged();
        card->OnConfigChanged();
        UiDriver::Step();
    }

    EXPECT_EQ(o2Scene.GetRootActors().Count(), actorsBefore);
    EXPECT_EQ(o2Scene.GetAllEditableObjects().Count(), editablesBefore);
}

// The right button pans the canvas over cards and their controls; a click without a pan opens the card menu
TEST_F(PipelineUiFixture, RightButtonPansOverCardsAndClicksOpenTheCardMenu)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    text->SetConfigString("text", "pan over me");
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);

    auto card = editor->GetNodeWidget(Live(text)->id);
    ASSERT_TRUE(card);
    editor->SetView(card->GetCardRect().Center(), 1.0f);
    UiDriver::Step(3);

    // Over the text area, the deepest control of the card
    auto area = card->FindChildByType<EditBox>();
    ASSERT_TRUE(area);
    Vec2F from = editor->LocalToScreenPoint(area->layout->GetWorldRect().Center());
    Vec2F cameraBefore = editor->GetCamera().GetPosition2D();
    o2Input.OnCursorMoved(from, 0, false);
    UiDriver::Step();
    o2Input.OnAltCursorPressed(from);
    UiDriver::Step();
    for (int i = 1; i <= 10; i++)
    {
        o2Input.OnCursorMoved(from + Vec2F(12.0f * i, 0.0f), 0);
        UiDriver::Step();
    }
    o2Input.OnAltCursorReleased();
    UiDriver::Wait(0.4f);

    EXPECT_GT((editor->GetCamera().GetPosition2D() - cameraBefore).Length(), 40.0f);
    EXPECT_FALSE(editor->GetContextMenu()->IsEnabled());

    // A click in place opens the menu of the card under the cursor
    Vec2F at = editor->LocalToScreenPoint(card->GetCardRect().Center());
    o2Input.OnCursorMoved(at, 0, false);
    UiDriver::Step();
    o2Input.OnAltCursorPressed(at);
    UiDriver::Step();
    o2Input.OnAltCursorReleased();
    UiDriver::Step(3);
    EXPECT_TRUE(card->IsSelected());
}

// The settings dialog is tall enough for its fields and the buttons below them
TEST_F(PipelineUiFixture, SettingsDialogFitsItsButtons)
{
    PipelineSettingsDlg::Show();
    UiDriver::Step(5);
    auto& dlg = PipelineSettingsDlg::Instance();
    auto window = dlg.GetWindow();
    ASSERT_TRUE(window);
    auto save = window->FindChildByTypeAndName<Button>("Save");
    ASSERT_TRUE(save);
    RectF windowRect = window->layout->GetWorldRect();
    RectF saveRect = save->layout->GetWorldRect();
    // The view area ends 5 units above the window bottom
    EXPECT_GE(saveRect.bottom, windowRect.bottom + 5.0f);
    EXPECT_LE(saveRect.top, windowRect.top);
    String dir = ScreenshotDir();
    o2FileSystem.FolderCreate(dir, true);
    EXPECT_TRUE(UiDriver::Screenshot(dir + "/pipeline_settings.png"));
    window->Hide(true);
}

// A second right click while the add menu is still open refills it in place; every category keeps all its nodes
TEST_F(PipelineUiFixture, AddMenuRefilledWhileOpenKeepsEverySubMenuItem)
{
    PipelineGraph graph;
    AddNode(graph, "sourceText", Vec2F());
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    editor->SetView(Vec2F(), 1.0f);
    UiDriver::Step(3);

    Vec2F at = editor->LocalToScreenPoint(Vec2F(-300, 200));
    auto rightClick = [&]()
    {
        o2Input.OnCursorMoved(at, 0, false);
        UiDriver::Step();
        o2Input.OnAltCursorPressed(at);
        UiDriver::Step();
        o2Input.OnAltCursorReleased();
        UiDriver::Step();
    };
    rightClick();
    auto menu = editor->GetContextMenu();
    ASSERT_TRUE(menu->IsEnabledInHierarchy());
    rightClick();
    UiDriver::Step(3);

    int aiNodes = 0;
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        if (schema->category == PipelineNodeCategory::AI)
            aiNodes++;
    }
    ASSERT_GT(aiNodes, 1);

    auto item = menu->FindChildByTypeAndName<ContextMenuItem>("Context Item Add AI");
    ASSERT_TRUE(item);
    ASSERT_TRUE(item->GetSubMenu());
    EXPECT_EQ(item->GetSubMenu()->GetItems().Count(), aiNodes);
    menu->Hide(true);
}

// The header fill, the body fill and the outline share one rect: nothing sticks out above the outline
TEST_F(PipelineUiFixture, CardHeaderAndOutlineShareOneEdge)
{
    PipelineGraph graph;
    auto text = AddNode(graph, "sourceText", Vec2F());
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    UiDriver::Step(3);
    auto card = editor->GetNodeWidget(Live(text)->id);
    ASSERT_TRUE(card);
    card->SetSelected(true);

    for (float scale : { 0.25f, 1.0f })
    {
        RectF rect = card->GetCardRect();
        editor->SetView(Vec2F(rect.left + 40.0f, rect.top - 30.0f), scale);
        UiDriver::Step(4);
        auto capture = UiDriver::Capture();
        ASSERT_TRUE(capture);
        String dir = ScreenshotDir();
        o2FileSystem.FolderCreate(dir, true);
        capture->Save(dir + "/pipeline_card_edge_" + (String)(int)(scale * 100) + ".png", Bitmap::ImageType::Png);

        // Just above the selection outline is the canvas, never a piece of the header
        Vec2I above = ScreenToCapture(editor->LocalToScreenPoint(Vec2F(rect.left + 60.0f, rect.top + 5.0f + 3.0f * scale)), capture);
        const UInt8* p = PipelineImageOps::Pixel(*capture, above.x, above.y);
        EXPECT_TRUE(IsBackgroundPixel(capture, above)) << "scale " << scale << " rgb " << (int)p[0] << " " << (int)p[1] << " " << (int)p[2];
    }
}
