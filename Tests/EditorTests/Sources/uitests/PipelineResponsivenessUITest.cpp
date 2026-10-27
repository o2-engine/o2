#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"
#include "o2/Application/VKCodes.h"
#include "o2/Assets/Assets.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerLayers.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageLoader.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

// The Pipeline window stays responsive: cards are freed when rebuilt, pictures decode in the background, a composer layer
// renamed keeps its card, an undo touches only the cards it changes

namespace
{
    double NowMs()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    double TimedStep()
    {
        double start = NowMs();
        Step();
        return NowMs() - start;
    }

    // A picture big enough that decoding it is felt, cheap to make
    Ref<Bitmap> BigPicture(const Color4& a, const Color4& b, int size = 2048)
    {
        auto bitmap = PipelineImageOps::Blank(size, size, a);
        for (int y = 0; y < size; y += 64)
        {
            for (int x = 0; x < size; x++)
            {
                UInt8* p = PipelineImageOps::Pixel(*bitmap, x, y);
                p[0] = (UInt8)b.r; p[1] = (UInt8)b.g; p[2] = (UInt8)b.b; p[3] = (UInt8)b.a;
            }
        }
        return bitmap;
    }

    struct PipelineResponsivenessUiFixture : PipelinePairUiFixture
    {
        // A work folder of its own: ctest runs the pair suite next to this one
        String WorkDir() const override { return "../../Work/Pipelines/uitest-responsive/"; }

        // Opens a composer fed by that many image sources; the sources read two big uploads
        Ref<PipelineNode> OpenComposerWithBigLayers(int layers)
        {
            String big[2] = { "uitest-big-a.png", "uitest-big-b.png" };
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(big[0]),
                                          PipelineValue::Image(BigPicture(Color4(200, 60, 40, 255), Color4(20, 20, 20, 255))).GetPngBytes());
            PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(big[1]),
                                          PipelineValue::Image(BigPicture(Color4(40, 90, 200, 255), Color4(240, 240, 240, 255))).GetPngBytes());

            PipelineGraph graph;
            graph.id = "uitest-responsive";
            auto composer = PipelineNodeRegistry::CreateNode("composer", Vec2F(0, 0));
            Vector<PipelinePort> customs;
            for (int i = 0; i < layers; i++)
                customs.Add(PipelinePort("in" + (String)i, "layer " + (String)i, PipelinePortType::Image, true));
            composer->SetCustomInputs(customs);
            PipelineNodeRegistry::SyncNodeWithSchema(composer);
            graph.nodes.Add(composer);

            for (int i = 0; i < layers; i++)
            {
                auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-500, i*300.0f));
                source->SetConfigString("uploadId", big[i % 2]);
                graph.nodes.Add(source);

                auto edge = mmake<PipelineEdge>();
                edge->id = PipelineNode::GenerateId();
                edge->fromNodeId = source->id;
                edge->fromPortId = source->outputs[0].id;
                edge->toNodeId = composer->id;
                edge->toPortId = "in" + (String)i;
                graph.edges.Add(edge);
            }

            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            editor->FinishBackgroundWork();
            Step(3);
            auto live = editor->GetGraph()->FindNode(composer->id);
            editor->SetView(editor->GetNodeWidget(live->id)->GetCardRect().Center(), 1.0f);
            Step(3);
            return live;
        }
    };
}

TEST_F(PipelineResponsivenessUiFixture, CardsAreFreedWhenRebuiltOrDeleted)
{
    PipelineGraph graph;
    graph.id = "uitest-responsive";
    int i = 0;
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        graph.nodes.Add(PipelineNodeRegistry::CreateNode(schema->type, Vec2F((i % 6)*300.0f, (i / 6)*700.0f)));
        i++;
    }
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    Step(3);

    Vector<Pair<String, WeakRef<PipelineNodeWidget>>> cards;
    for (auto& node : editor->GetGraph()->nodes)
        cards.Add({ node->nodeType, WeakRef<PipelineNodeWidget>(editor->GetNodeWidget(node->id)) });

    editor->RebuildAll();
    Step(4);
    for (auto& card : cards)
        EXPECT_FALSE(card.second.Lock()) << card.first.Data() << ": the card outlived a rebuild";

    auto victim = editor->GetGraph()->nodes[0]->id;
    WeakRef<PipelineNodeWidget> deleted(editor->GetNodeWidget(victim));
    editor->SelectNodes({ victim });
    editor->DeleteSelection();
    Step(4);
    EXPECT_FALSE(deleted.Lock());
}

TEST_F(PipelineResponsivenessUiFixture, PicturesArriveFromTheBackground)
{
    auto node = OpenFed("imageOutline", 480.0f, true, BigPicture(Color4(30, 160, 60, 255), Color4(0, 0, 0, 255), 1600));
    auto card = editor->GetNodeWidget(node->id);
    editor->FinishBackgroundWork();
    Step(2);
    ASSERT_TRUE(card->GetBody()->GetPair());

    // Opening again: the cards come up at once, the pictures follow
    editor->SetAsset(nullptr);
    Step();
    editor->SetAsset(asset);
    card = editor->GetNodeWidget(node->id);
    EXPECT_TRUE(editor->GetBackgroundStatus().StartsWith("Loading previews")) << editor->GetBackgroundStatus().Data();
    EXPECT_TRUE(editor->IsWaitingForPictures(node->id));
    EXPECT_FALSE(card->GetRuntime().output.PeekBitmap());

    editor->FinishBackgroundWork();
    Step(2);
    EXPECT_TRUE(card->GetRuntime().output.PeekBitmap());
    EXPECT_FALSE(editor->IsWaitingForPictures(node->id));
    EXPECT_TRUE(editor->GetBackgroundStatus().IsEmpty());
}

TEST_F(PipelineResponsivenessUiFixture, AnImageEditCanvasWaitsForItsPicture)
{
    uploadId = "uitest-big-edit.png";
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(uploadId),
                                  PipelineValue::Image(BigPicture(Color4(90, 90, 200, 255), Color4(0, 0, 0, 255))).GetPngBytes());
    auto node = OpenFed("imageEdit", 480.0f, true);
    editor->FinishBackgroundWork();

    // Opened again, the picture decodes while the card is up: the canvas is not laid out at the card's size meanwhile,
    // which the picture would then resample
    editor->SetAsset(nullptr);
    Step();
    editor->SetAsset(asset);
    auto paint = editor->GetNodeWidget(node->id)->FindChildByType<PipelinePaintEditor>();
    ASSERT_TRUE(paint);
    int waited = 0;
    for (; waited < 600 && editor->IsWaitingForPictures(node->id); waited++)
    {
        EXPECT_EQ(paint->GetResolution(), Vec2I()) << "frame " << waited;
        Step();
    }
    EXPECT_GT(waited, 0);
    Step(2);
    ASSERT_TRUE(paint->GetBackground());
    EXPECT_EQ(paint->GetResolution(), Vec2I(2048, 2048));
}

TEST_F(PipelineResponsivenessUiFixture, AssetsAreRebuiltInTheBackground)
{
    o2Assets.RebuildAssetsInBackground();
    EXPECT_TRUE(editor->GetBackgroundStatus().Contains("Rebuilding assets")) << editor->GetBackgroundStatus().Data();
    EXPECT_LT(TimedStep(), 200.0);

    editor->FinishBackgroundWork();
    EXPECT_FALSE(o2Assets.IsRebuildingAssets());
    EXPECT_TRUE(editor->GetBackgroundStatus().IsEmpty()) << editor->GetBackgroundStatus().Data();
}

TEST_F(PipelineResponsivenessUiFixture, CopiesOfAShownValueShareOneDecode)
{
    auto value = PipelineValue::ImageBytes(PipelineValue::Image(PipelineImageOps::Blank(64, 32, inputColor)).GetPngBytes());
    value.ShareDecoded();
    PipelineValue copy = value;
    auto decoded = copy.GetBitmap();
    ASSERT_TRUE(decoded);
    EXPECT_EQ(value.PeekBitmap(), decoded);

    PipelineImageLoader loader;
    auto other = PipelineValue::ImageBytes(PipelineValue::Image(PipelineImageOps::Blank(1500, 20, inputColor)).GetPngBytes());
    other.ShareDecoded();
    EXPECT_FALSE(loader.Request(other, "a"));
    EXPECT_FALSE(loader.Request(PipelineValue(other), "b"));
    EXPECT_EQ(loader.GetPendingCount(), 1);
    EXPECT_TRUE(loader.IsWaiting("a") && loader.IsWaiting("b"));
    auto ready = loader.WaitAll();
    EXPECT_TRUE(ready.Contains("a") && ready.Contains("b"));
    ASSERT_TRUE(other.PeekBitmap());
    EXPECT_EQ(other.PeekBitmap()->GetSize(), Vec2I(1500, 20));
    EXPECT_FALSE(loader.IsWaiting("a"));

    // The cards draw a copy no larger than a thousand pixels, made with the decode
    Vec2I size;
    EXPECT_TRUE(PipelineDisplayCache::GetTexture(other.PeekBitmap(), &size));
    EXPECT_EQ(size.x, PipelineDisplayCache::maxSide);
}

TEST_F(PipelineResponsivenessUiFixture, RenamingAComposerLayerKeepsTheCardAndStaysFast)
{
    auto composer = OpenComposerWithBigLayers(10);
    auto card = editor->GetNodeWidget(composer->id);
    auto body = card->GetBody();
    auto panel = card->FindChildByTypeAndName<PipelineComposerLayersPanel>("layers panel");
    ASSERT_TRUE(panel);

    Map<String, Ref<Widget>> rows;
    for (int i = 0; i < 10; i++)
        rows["in" + (String)i] = panel->FindRow("in" + (String)i);

    auto name = rows["in3"]->FindChildByTypeAndName<EditBox>("layer name");
    ASSERT_TRUE(name);
    name->Focus();
    Step(2);
    ASSERT_TRUE(name->IsFocused());

    // A focused single-line field selects its text: the caret goes to the end to type after the name
    int length = name->GetText().Length();
    name->Select(length, length);

    double typing = 0.0;
    for (char c : String(" hills"))
    {
        KeyboardKey key = c == ' ' ? VK_SPACE : (KeyboardKey)toupper((unsigned char)c);
        o2Input.OnKeyPressed(key);
        typing = Math::Max(typing, TimedStep());
        o2Input.OnKeyReleased(key);
        typing = Math::Max(typing, TimedStep());
    }

    o2Input.OnKeyPressed(VK_RETURN);
    double commit = TimedStep();
    o2Input.OnKeyReleased(VK_RETURN);
    commit = Math::Max(commit, TimedStep());
    for (int i = 0; i < 30; i++)
        commit = Math::Max(commit, TimedStep());

    // The letters typed follow the machine's keyboard layout
    String renamed = composer->GetCustomInputs().FindOrDefault([](const PipelinePort& p) { return p.id == "in3"; }).name;
    EXPECT_EQ(renamed, (String)name->GetText());
    EXPECT_GT(renamed.Length(), String("layer 3").Length()) << renamed.Data();

    // The card, its body and every row are the ones that were there
    EXPECT_EQ(editor->GetNodeWidget(composer->id), card);
    EXPECT_EQ(card->GetBody(), body);
    for (auto& row : rows)
        EXPECT_EQ(panel->FindRow(row.first), row.second) << row.first.Data();

    printf("[responsive] composer layer rename: typing frames up to %.1f ms, the commit and its consequences up to %.1f ms\n", typing, commit);
    EXPECT_LT(typing, 60.0);
    EXPECT_LT(commit, 200.0);
}

TEST_F(PipelineResponsivenessUiFixture, UndoAndDeleteTouchOnlyTheirCards)
{
    PipelineGraph graph;
    graph.id = "uitest-responsive";
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F(-400, 0));
    source->SetConfigString("uploadId", uploadId);
    graph.nodes.Add(source);
    Vector<Ref<PipelineNode>> outlines;
    for (int i = 0; i < 4; i++)
    {
        auto outline = PipelineNodeRegistry::CreateNode("imageOutline", Vec2F(0, i*500.0f));
        graph.nodes.Add(outline);
        outlines.Add(outline);
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = source->id;
        edge->fromPortId = source->outputs[0].id;
        edge->toNodeId = outline->id;
        edge->toPortId = outline->inputs[0].id;
        graph.edges.Add(edge);
    }
    graph.SaveToAsset(*asset);
    auto actions = mmake<ActionsList>();
    editor->actionsListDelegate = actions;
    editor->SetAsset(asset);
    editor->FinishBackgroundWork();
    Step(3);

    Map<String, Ref<PipelineNodeWidget>> cards;
    for (auto& node : editor->GetGraph()->nodes)
        cards[node->id] = editor->GetNodeWidget(node->id);

    // A parameter edited and undone rebuilds its own card only
    auto changed = outlines[1]->id;
    auto changedBody = cards[changed]->GetBody();
    Map<String, Ref<PipelineNodeBody>> bodies;
    for (auto& card : cards)
        bodies[card.first] = card.second->GetBody();

    float original = editor->GetGraph()->FindNode(changed)->GetConfigNumber("width", -1.0f);
    editor->BeginContinuousEdit("Edit width");
    editor->GetGraph()->FindNode(changed)->SetConfigNumber("width", 11);
    editor->EndContinuousEdit();
    actions->UndoAction();
    Step(2);
    EXPECT_EQ(editor->GetGraph()->FindNode(changed)->GetConfigNumber("width", -1.0f), original);
    for (auto& card : cards)
    {
        EXPECT_EQ(editor->GetNodeWidget(card.first), card.second);
        if (card.first != changed)
            EXPECT_EQ(card.second->GetBody(), bodies[card.first]);
    }
    EXPECT_NE(cards[changed]->GetBody(), changedBody);

    // Deleting a card leaves the others; undoing brings it back with its picture's source
    auto otherBody = cards[outlines[2]->id]->GetBody();
    editor->SelectNodes({ outlines[3]->id });
    editor->DeleteSelection();
    Step(2);
    EXPECT_FALSE(editor->GetNodeWidget(outlines[3]->id));
    EXPECT_EQ(editor->GetNodeWidget(outlines[2]->id)->GetBody(), otherBody);

    actions->UndoAction();
    editor->FinishBackgroundWork();
    Step(2);
    ASSERT_TRUE(editor->GetNodeWidget(outlines[3]->id));
    EXPECT_EQ(editor->GetGraph()->GetIncomingEdges(outlines[3]->id).Count(), 1);
    EXPECT_EQ(editor->GetNodeWidget(outlines[2]->id)->GetBody(), otherBody);
}

TEST_F(PipelineResponsivenessUiFixture, TheCameraReachesTheDocumentOnceItRests)
{
    OpenFed("imageOutline", 480.0f, true);
    Step(30);

    auto documentScale = [&]()
    {
        auto camera = asset->document.FindMember("camera");
        auto scale = camera ? camera->FindMember("scale") : nullptr;
        return scale ? PipelineUtils::ValueToNumber(*scale, 0.0f) : 0.0f;
    };

    // Zooming writes the graph's camera every frame and the whole document only after the view rests
    float before = documentScale();
    Vec2F center = editor->LocalToScreenPoint(editor->GetVisibleCanvasRect().Center());
    o2Input.OnCursorMoved(center, 0, false);
    for (int i = 0; i < 5; i++)
    {
        o2Input.OnMouseWheel(120.0f);
        Step();
        EXPECT_FLOAT_EQ(documentScale(), before) << "frame " << i;
    }
    EXPECT_NE(editor->GetGraph()->cameraScale, 1.0f/Math::Max(before, 0.0001f));

    Step(40);
    EXPECT_NE(documentScale(), before);
    EXPECT_NEAR(documentScale(), 1.0f/editor->GetGraph()->cameraScale, 0.001f);
}

TEST_F(PipelineResponsivenessUiFixture, CardsOutOfViewAreLaidOutAFewAFrame)
{
    PipelineGraph graph;
    graph.id = "uitest-responsive";
    for (int i = 0; i < 40; i++)
        graph.nodes.Add(PipelineNodeRegistry::CreateNode(i % 2 ? "imageOutline" : "promptGen", Vec2F((i % 8)*2000.0f, (i / 8)*3000.0f)));
    graph.SaveToAsset(*asset);
    editor->SetAsset(asset);
    editor->SetView(editor->GetNodeWidget(editor->GetGraph()->nodes[0]->id)->GetCardRect().Center(), 1.0f);
    Step();

    int culled = 0;
    for (auto& node : editor->GetGraph()->nodes)
        culled += editor->GetNodeWidget(node->id)->IsCulled() ? 1 : 0;
    ASSERT_GT(culled, 30);

    Step(240);
    for (auto& node : editor->GetGraph()->nodes)
    {
        auto card = editor->GetNodeWidget(node->id);
        EXPECT_TRUE(card->IsWarm()) << node->nodeType.Data() << (card->IsCulled() ? " out of view" : "");
    }
}
