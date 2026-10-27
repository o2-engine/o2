#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <set>
#include <unistd.h>

#include "PipelinePairUiFixture.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Render/Pipeline/ScenePasses.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/PipelineWindow/ImageToUiDebugDlg.h"
#include "tests/Pipeline/ImageToUiTestKit.h"

// AI image to UI in the editor: the models stubbed and the agent scripted (ImageToUiTestKit) to drive the real skill through
// the executor, the card, the debug view and the prototype export. Screenshots go to Work/Pipelines/shots/pairs/

namespace
{
    using UiLayout::Json;

    String MetaId(const String& path)
    {
        Json meta;
        if (!Json::Parse(PipelineUtils::ReadFileBytes(path + ".meta"), meta))
            return "";
        auto value = meta.Get("Value");
        return value && value->Get("mId") ? value->Get("mId")->Str() : String();
    }

    // The actor ids of a saved prototype, in tree order
    std::vector<std::string> ActorIds(const String& proto)
    {
        std::vector<std::string> ids;
        Function<void(const Json&)> walk = [&](const Json& json)
        {
            if (json.IsObject())
            {
                for (auto& member : json.Members())
                {
                    if (member.first == "Id")
                        ids.push_back(member.second.ToJsString().Data());
                    walk(member.second);
                }
            }
            else if (json.IsArray())
            {
                for (auto& item : json.Items())
                    walk(item);
            }
        };
        Json json;
        if (Json::Parse(proto, json))
            walk(json);
        return ids;
    }

    void RemoveTestAssets(const String& folder)
    {
        o2FileSystem.FolderRemove(o2Assets.GetAssetsPath() + folder, true);
        o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + folder + ".meta");
    }

    // The share of a frame the scene paints over the clear colour, drawn the way the scene view draws it (Scene2DPass over
    // the visible layers); the frame is saved under shotName
    float SceneShare(const String& shotName)
    {
        const Color4 clear(30, 31, 34, 255);
        Ref<Bitmap> shot;
        for (int i = 0; i < 6; i++)
        {
            ForcePopEditorScopeOnStack sceneScope;
            if (i == 5)
                o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { shot = bitmap; });
            o2Scene.Update(1.0f/60.0f);
            o2Render.Begin();
            Camera camera;
            o2Render.SetCamera(camera);
            o2Render.Clear(clear);
            o2Scene.BeginDrawingScene();
            RenderPassContext context;
            context.camera = camera;
            for (auto& layer : o2Scene.GetLayers())
            {
                if (layer->IsVisible())
                    context.layers.Add(layer);
            }
            Scene2DPass pass;
            pass.Execute(context);
            o2Scene.EndDrawingScene();
            o2Render.End();
            AppTestDriver::PumpFrames(1);
        }
        if (!shot)
            return 0.0f;
        shot->Save("../../Work/Pipelines/shots/" + shotName + ".png", Bitmap::ImageType::Png);
        int painted = 0, total = 0;
        for (int y = 0; y < shot->GetSize().y; y += 2)
        {
            for (int x = 0; x < shot->GetSize().x; x += 2)
            {
                const UInt8* c = PipelineImageOps::Pixel(*shot, x, y);
                total++;
                if (Math::Abs(c[0] - clear.r) + Math::Abs(c[1] - clear.g) + Math::Abs(c[2] - clear.b) > 30)
                    painted++;
            }
        }
        return total ? (float)painted/total : 0.0f;
    }

    void WaitRun(const Ref<PipelineEditor>& editor, int maxFrames)
    {
        for (int i = 0; i < maxFrames && editor->IsRunning(); i++)
            Step();
        Step(3);
    }
}

namespace
{
    // A fixture of its own: the pair fixture lives in an anonymous namespace, one class per file that includes it
    struct ImageToUiUiFixture: PipelinePairUiFixture
    {
        String WorkDir() const override { return "../../Work/Pipelines/uitest-imagetoui/"; }
    };
}

// One run makes every output, the card says the skill completed and shows the sprites with their ports, the prototype is
// written, the debug view lists the agent's turns with design | reconstruction after the check, Save to Assets keeps the
// prototype's uid and a second run is served by the cache
TEST_F(ImageToUiUiFixture, RunsTheSkillThroughTheEditor)
{
    if (UiLayout::ImageToUiHome().IsEmpty() || ImageToUiTestKit::PythonWithDependencies().IsEmpty())
        GTEST_SKIP() << "no o2/Tools/ImageToUi or no Python 3 with Pillow and numpy";

    PushEditorScopeOnStack editorScope;
    if (!AssetsWindow::IsSingletonInitialzed())
        mmake<AssetsWindow>();
    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    const String folder = "Generated/uitest-image-to-ui";
    RemoveTestAssets(folder);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(uploadId), ImageToUiTestKit::MockupPng());

    auto node = OpenFed("imageToUi");
    node->SetConfigString("assetPath", folder);
    node->SetConfigString("screenName", "UiTest");
    node->SetConfigString("sprites", "ai");
    node->SetConfigNumber("maxTurns", 20);
    editor->GetNodeWidget(node->id)->Rebuild();
    Step(3);
    ImageToUiTestKit::StubModels models(PipelineExecutor::GetWorkPath(editor->GetPipelineId(), node->id) + "task/");
    ShotCard(node, "image_to_ui_before");

    editor->RunNode(node->id, false);
    Step(30);
    EXPECT_TRUE(editor->IsRunning());
    ShotCard(node, "image_to_ui_running");
    WaitRun(editor, 60*900);
    ASSERT_FALSE(editor->IsRunning());

    auto card = editor->GetNodeWidget(node->id);
    ASSERT_TRUE(card);
    auto& rt = card->GetRuntime();
    ASSERT_EQ(rt.state, String("idle")) << rt.error.Data();
    EXPECT_EQ(models.agent.Load(), 4);
    auto live = editor->GetGraph()->FindNode(node->id);

    ASSERT_EQ(live->outputs.Count(), 5);
    EXPECT_EQ(live->outputs[0].name, String("preview"));
    EXPECT_EQ(live->outputs[1].name, String("layout"));
    auto sprites = PipelineImageToUi::SpritePorts(*live);
    std::set<std::string> names;
    for (auto& port : sprites)
    {
        names.insert(port.name.Data());
        EXPECT_TRUE(rt.portOutputs.ContainsKey(port.id)) << port.name.Data() << ": the run gave it a value";
    }
    EXPECT_EQ(names, std::set<std::string>({ "button_red", "icon_tile", "panel_blue" }));

    // The card: completed, the counts
    PipelineValue layout;
    ASSERT_TRUE(rt.portOutputs.TryGetValue(live->outputs[1].id, layout));
    auto summary = PipelineImageToUi::SummarizeLayout(layout.data);
    ASSERT_TRUE(summary.has_value());
    String traceDir = PipelineExecutor::GetWorkPath(editor->GetPipelineId(), node->id) + "trace/";
    auto run = PipelineImageToUi::ReadRunSummary(PipelineUtils::ReadFileBytes(traceDir + "trace.json"));
    ASSERT_TRUE(run.has_value());
    EXPECT_TRUE(run->complete);
    auto label = card->FindChildByTypeAndName<Label>("summary");
    ASSERT_TRUE(label);
    EXPECT_EQ(label->GetText(), WString(PipelineImageToUi::SummaryLine(*summary, &*run, sprites.Count())));
    EXPECT_TRUE(((String)label->GetText()).StartsWith("completed \xC2\xB7 1 checks"));

    // The sprite ports sit on their cells, the preview and the layout keep their port rows
    auto body = card->GetBody();
    ASSERT_TRUE(body);
    EXPECT_FALSE(body->HasBodyPort(live->outputs[0].id));
    for (auto& port : sprites)
        EXPECT_TRUE(body->HasBodyPort(port.id)) << port.name.Data();
    ShotCard(node, "image_to_ui_after");

    // The prototype and its resources
    String proto = folder + "/UiTest.proto";
    EXPECT_TRUE(o2Assets.IsAssetExist(proto));
    EXPECT_TRUE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + folder + "/UiTest_Resources/Fonts/Roboto-Bold.ttf"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + folder + "/UiTest_Resources/Sprites/icon_tile.png"));

    // The debug view: every step listed, the check under the compare divider, a step without a reconstruction without it
    Json trace;
    ASSERT_TRUE(Json::Parse(PipelineUtils::ReadFileBytes(traceDir + "trace.json"), trace));
    String checkId;
    for (auto& step : trace.Get("steps")->Items())
    {
        if (checkId.IsEmpty() && step.Get("kind")->Str() == "check")
            checkId = step.Get("id")->Str();
    }
    ASSERT_FALSE(checkId.IsEmpty());
    auto debug = FindToggle(card, "Debug");
    ASSERT_TRUE(debug);
    debug->SetValue(true);
    debug->onToggleByUser(true);
    Step(5);
    EXPECT_TRUE(ImageToUiDebug::IsShownFor(node->id));
    EXPECT_EQ(ImageToUiDebug::GetStepIds().Count(), trace.Get("steps")->Count());
    ImageToUiDebug::SelectStep(checkId);
    Step(5);
    auto window = ImageToUiDebug::GetWindow();
    ASSERT_TRUE(window);
    EXPECT_TRUE(window->FindChildByTypeAndName<PipelineCompareView>("debug compare"));
    EXPECT_TRUE(window->FindChildByTypeAndName<PipelineTextView>("debug action"));
    ImageToUiDebug::SelectStep("analysis");
    Step(5);
    EXPECT_FALSE(window->FindChildByTypeAndName<PipelineCompareView>("debug compare"));
    window->Hide(true);
    Step(3);

    // Save to Assets writes the prototype again in place
    String protoPath = o2Assets.GetAssetsPath() + proto;
    String uid = MetaId(protoPath), before = PipelineUtils::ReadFileBytes(protoPath);
    ASSERT_FALSE(uid.IsEmpty());
    ASSERT_TRUE(card->GetBody()->SaveToAssets());
    auto save = card->FindChildByTypeAndName<Button>("save to assets");
    ASSERT_TRUE(save);
    Step(2);
    for (int i = 0; i < 60*300 && !save->IsInteractable(); i++)
        Step();
    EXPECT_EQ(MetaId(protoPath), uid) << "the prototype keeps its uid";
    EXPECT_EQ(ActorIds(PipelineUtils::ReadFileBytes(protoPath)), ActorIds(before)) << "and its actor ids";

    // The written prototype draws when opened the way the Assets window opens one (a clone of the template into the scene)
    {
        Ref<Actor> opened;
        bool hadLayer = o2Scene.HasLayer("UI");
        {
            ForcePopEditorScopeOnStack sceneScope;
            AssetRef<ActorAsset> asset(proto);
            ASSERT_TRUE(asset);
            opened = asset->GetActor()->CloneAsRef<Actor>();
            o2Scene.UpdateAddedEntities();
        }
        float share = SceneShare("image_to_ui_opened_prototype");
        EXPECT_GT(share, 0.02f) << "the prototype's sprites and texts are drawn";
        ForcePopEditorScopeOnStack sceneScope;
        opened->Destroy();
        opened = nullptr;
        o2Scene.UpdateDestroyingEntities();
        if (!hadLayer)
            o2Scene.RemoveLayer("UI");
    }

    // The bundle serves the next run: the models are not asked again
    editor->RunNode(node->id, false);
    WaitRun(editor, 60*300);
    EXPECT_EQ(models.agent.Load(), 4);

    RemoveTestAssets(folder);
    o2Assets.RebuildAssets();
}

// A folder tree that is new to the assets appears whole after one rebuild, not one level per rebuild
TEST(AssetsRebuild, NewNestedFoldersAppearAtOnce)
{
    if (!o2FileSystem.IsFileExist("./AssetsBuilder") && o2FileSystem.IsFileExist("../../Bin/Mac/AssetsBuilder"))
        symlink("../../Bin/Mac/AssetsBuilder", "./AssetsBuilder");

    const String folder = "Generated/uitest-nested";
    RemoveTestAssets(folder);
    String root = o2Assets.GetAssetsPath();
    o2FileSystem.FolderCreate(root + folder + "/a/b", true);
    PipelineUtils::WriteFileBytes(root + folder + "/a/b/c.txt", "nested");
    auto meta = [](const String& type)
    {
        UID uid;
        uid.Randomize();
        return "{\n    \"Type\": \"" + type + "\",\n    \"Value\": {\n        \"mId\": \"" + String(uid.ToString()) + "\"\n    }\n}";
    };
    for (auto& path : { folder, folder + "/a", folder + "/a/b" })
        PipelineUtils::WriteFileBytes(root + path + ".meta", meta("o2::DefaultAssetMeta<o2::FolderAsset>"));
    PipelineUtils::WriteFileBytes(root + folder + "/a/b/c.txt.meta", meta("o2::DefaultAssetMeta<o2::BinaryAsset>"));

    o2Assets.RebuildAssets();
    EXPECT_TRUE(o2Assets.IsAssetExist(folder + "/a"));
    EXPECT_TRUE(o2Assets.IsAssetExist(folder + "/a/b"));
    EXPECT_TRUE(o2Assets.IsAssetExist(folder + "/a/b/c.txt"));

    RemoveTestAssets(folder);
    o2Assets.RebuildAssets();
    EXPECT_FALSE(o2Assets.IsAssetExist(folder + "/a/b/c.txt"));
}
