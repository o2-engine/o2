#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2Editor/Pipeline/UiDoc/UiDocExport.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"

// The AI UI layout's Save to project: whatever the node's exportFor says (the web's download target), the card and the
// workspace write an o2 prototype into the project's Assets - the labels in the style's font and its look as a font
// style - and leave exportFor as it was; the card shows where the prototype is

namespace
{
    const char* const kSaveFolder = "Generated/uitest-ui-save";

    // The node's own title style: Lobster, a gradient, an outline and a drop shadow
    UiDoc::Json TitleStyle()
    {
        UiDoc::Json title;
        EXPECT_TRUE(UiDoc::Json::Parse(R"({"name":"title","role":"title","font":"Lobster","weight":400,"size":72,"color":"#FFF0A0",
            "fill":{"angle":90,"stops":[{"color":"#FFF0A0","at":0},{"color":"#FF8000","at":1}]},
            "effects":[{"type":"stroke","color":"#402000","width":4,"opacity":1},
                       {"type":"shadow","color":"#000000","x":0,"y":6,"blur":4,"spread":0,"opacity":0.5}]})", title));
        return title;
    }

    struct UiLayoutSaveFixture: UiWorkspaceFixture
    {
        String WorkDir() const override { return "../../Work/Pipelines/uitest-uisave/"; }

        void TearDown() override
        {
            RemoveSaved();
            UiWorkspaceFixture::TearDown();
        }

        void RemoveSaved()
        {
            o2FileSystem.FolderRemove(o2Assets.GetAssetsPath() + kSaveFolder, true);
            o2FileSystem.FileDelete(o2Assets.GetAssetsPath() + kSaveFolder + ".meta");
            o2Assets.RebuildAssets();
        }

        Ref<PipelineNodeWidget> Card() const { return editor->GetNodeWidget(layoutNode->id); }

        // Opens the layout with the title style its own (made by its agent step) and the card's parameters unfolded
        void OpenStyled()
        {
            OpenWorkspace(false);
            auto steps = StepsNow();
            UiDoc::Json styles = UiDoc::Json::Array();
            styles.Add(TitleStyle());
            steps[0].data["styles"] = styles;
            UiDoc::WriteSteps(layoutNode->config, steps, {});
            layoutNode->SetConfigString("assetPath", kSaveFolder);
            layoutNode->SetConfigBool("paramsOpen", true);
            Card()->GetBody()->RebuildBody();
            UiWorkspace::Refresh(layoutNode->id);
            Step(3);
        }

        void WaitSaved()
        {
            Step(2);
            for (int i = 0; i < 60*120 && PipelineUiAi::IsBusy("export", layoutNode->id); i++)
                Step();
            Step(2);
        }
    };
}

TEST_F(UiLayoutSaveFixture, SaveToProjectWritesAnO2PrototypeAndLeavesExportForAsItWas)
{
    OpenStyled();
    String proto = String(kSaveFolder) + "/Menu.proto";

    // No export target on the card: the folder and the save button
    auto card = Card();
    EXPECT_FALSE(card->FindChildByTypeAndName<Toggle>("export unity"));
    EXPECT_FALSE(card->FindChildByTypeAndName<Toggle>("export json"));
    auto folder = card->FindChildByTypeAndName<EditBox>("asset folder");
    ASSERT_TRUE(folder);
    EXPECT_EQ((String)folder->GetText(), String(kSaveFolder));
    auto save = card->FindChildByTypeAndName<Button>("save to project");
    ASSERT_TRUE(save);
    auto saved = card->FindChildByTypeAndName<Label>("saved path");
    ASSERT_TRUE(saved);
    EXPECT_EQ((String)saved->GetText(), "Not saved yet \xC2\xB7 Assets/" + proto);

    // From the card, the node's target saying Unity
    layoutNode->SetConfigString("exportFor", "unity");
    save->onClick();
    WaitSaved();
    EXPECT_TRUE(PipelineUiAi::LastError("export", layoutNode->id).IsEmpty()) << PipelineUiAi::LastError("export", layoutNode->id).Data();
    ASSERT_TRUE(o2Assets.IsAssetExist(proto));
    EXPECT_EQ(layoutNode->GetConfigString("exportFor", ""), String("unity"));
    saved = Card()->FindChildByTypeAndName<Label>("saved path");
    ASSERT_TRUE(saved);
    EXPECT_EQ((String)saved->GetText(), "Assets/" + proto);
    EXPECT_FALSE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + "Menu.unitypackage"));

    // The title is drawn in the style's font with its look
    {
        AssetRef<ActorAsset> asset(proto);
        ASSERT_TRUE(asset && asset->GetActor());
        Ref<Label> title;
        std::function<void(const Ref<Actor>&)> find = [&](const Ref<Actor>& actor)
        {
            if (auto label = DynamicCast<Label>(actor); label && actor->GetName() == "title")
                title = label;
            for (auto& child : actor->GetChildren())
                find(child);
        };
        find(asset->GetActor());
        ASSERT_TRUE(title);
        ASSERT_TRUE(title->GetFontAsset());
        EXPECT_TRUE(title->GetFontAsset()->GetPath().EndsWith("Fonts/Lobster-Regular.ttf")) << title->GetFontAsset()->GetPath().Data();
        ASSERT_TRUE(title->GetFontStyleAsset());
        EXPECT_TRUE(title->GetFontStyleAsset()->GetPath().Contains("FontStyles/text_")) << title->GetFontStyleAsset()->GetPath().Data();
        auto& effects = title->GetFontStyleAsset()->GetEffects();
        ASSERT_EQ(effects.Count(), 3);
        EXPECT_TRUE(DynamicCast<FontGradientEffect>(effects[0]));
        EXPECT_TRUE(DynamicCast<FontStrokeEffect>(effects[1]));
        EXPECT_TRUE(DynamicCast<FontShadowEffect>(effects[2]));
    }
    EXPECT_TRUE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + kSaveFolder + "/Menu_Resources/Fonts/LICENSE-Lobster.txt"));

    // From the workspace, the target saying JSON: the same prototype written again in place
    String protoPath = o2Assets.GetAssetsPath() + proto;
    UiDoc::Json meta;
    ASSERT_TRUE(UiDoc::Json::Parse(PipelineUtils::ReadFileBytes(protoPath + ".meta"), meta));
    String uid = meta.Get("Value")->Get("mId")->Str();
    layoutNode->SetConfigString("exportFor", "json");
    ASSERT_TRUE(Window()->onSave);
    Window()->onSave();
    WaitSaved();
    EXPECT_TRUE(PipelineUiAi::LastError("export", layoutNode->id).IsEmpty());
    EXPECT_EQ(layoutNode->GetConfigString("exportFor", ""), String("json"));
    ASSERT_TRUE(UiDoc::Json::Parse(PipelineUtils::ReadFileBytes(protoPath + ".meta"), meta));
    EXPECT_EQ(meta.Get("Value")->Get("mId")->Str(), uid) << "the prototype keeps its uid";
    EXPECT_FALSE(o2FileSystem.IsFileExist(o2Assets.GetAssetsPath() + "Menu.zip"));
}
