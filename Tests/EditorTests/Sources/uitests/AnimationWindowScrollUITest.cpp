#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/ITrackControl.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct AnimationWindowProbe: AnimationWindow
    {
        AnimationWindowProbe(RefCounter* refCounter): AnimationWindow(refCounter) {}

        const Ref<DockableWindow>& Window() const { return mWindow; }
        const Ref<AnimationTimeline>& Timeline() const { return mTimeline; }
        const Ref<AnimationTree>& Tree() const { return mTree; }
    };

    template<typename WidgetType>
    void CollectWidgets(const Ref<Widget>& root, Vector<Ref<WidgetType>>& result)
    {
        for (auto& child : root->GetChildWidgets())
        {
            if (auto typed = DynamicCast<WidgetType>(child))
                result.Add(typed);

            CollectWidgets(child, result);
        }
    }

    // Every visible row must own exactly one track control child that is enabled in hierarchy
    void ExpectRowControls(const Ref<AnimationTree>& tree, const char* stage)
    {
        Vector<Ref<AnimationTreeNode>> nodes;
        CollectWidgets(DynamicCast<Widget>(tree), nodes);
        for (auto& node : nodes)
        {
            Vector<Ref<ITrackControl>> controls;
            for (auto& child : node->GetChildWidgets())
            {
                if (auto control = DynamicCast<ITrackControl>(child))
                    controls.Add(control);
            }
            auto text = node->GetLayerDrawable<Text>("name");
            String label = text ? (String)text->GetText() : String("?");
            EXPECT_EQ(controls.Count(), 1) << stage << " row " << label;

            // nothing but real children may be drawn by the row (cached controls of scrolled-out rows)
            for (auto& drawable : node->GetChildrenInheritedDepth())
            {
                auto drawn = DynamicCast<Widget>(drawable);
                EXPECT_TRUE(drawn && node->GetChildWidgets().Contains(drawn))
                    << stage << " row " << label << ": a detached control is still drawn by the row";
            }
            for (auto& control : controls)
            {
                EXPECT_TRUE(control->IsEnabledInHierarchy()) << stage << " row " << label << ": pooled control stays disabled";
                EXPECT_GE(control->GetKeyHandles().Count(), 3) << stage << " row " << label;
            }
        }
    }

    Ref<Bitmap> DrawRootAndCapture()
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.SetCamera(Camera());
        o2Render.Clear(Color4(235, 235, 235));
        EditorUIRoot.GetRootWidget()->Draw();
        o2Render.End();

        return captured;
    }
}

// Track curves belong to their rows: after scrolling the tree nothing of them may be drawn
// above the rows area (over the ruler band)
TEST(AnimationWindowScrollUI, ScrolledTreeKeepsTrackLinesInsideRows)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    auto animation = widget->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    for (int i = 0; i < 25; i++)
    {
        String name = String("l") + (String)i;
        widget->AddLayer(name, mmake<Sprite>(Color4::White()), Layout::BothStretch());
        auto track = clip->AddTrack<float>("layer/" + name + "/transparency");
        track->AddKey(0.0f, 1.0f);
        track->AddKey(0.5f, 0.0f);
        track->AddKey(1.0f, 1.0f);
    }

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("fade");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 400.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(760.0f, 360.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        window->Show();
        window->Window()->SetEnabledForcible(false);
        window->Window()->SetEnabledForcible(true);

        // the window fades in through its visible state: pump the UI root until it is drawn
        for (int i = 0; i < 12; i++)
        {
            root->Update(0.1f);
            root->UpdateChildren(0.1f);
            root->UpdateChildrenTransforms();
        }
        ASSERT_TRUE(window->Window()->IsEnabledInHierarchy())
            << "root " << (int)root->IsEnabledInHierarchy() << " window " << (int)window->Window()->IsEnabled()
            << " parent " << (window->Window()->GetParent() ? window->Window()->GetParent().Lock()->GetName() : String("none"))
            << " parentEnabled " << (window->Window()->GetParent() ? (int)window->Window()->GetParent().Lock()->IsEnabledInHierarchy() : -1)
            << " rootIsParent " << (int)(window->Window()->GetParent().Lock() == root);

        ExpectRowControls(window->Tree(), "before");
        auto before = DrawRootAndCapture();
        ASSERT_TRUE(before);
        o2FileSystem.FolderCreate("TestScreenshots", true);
        before->Save("TestScreenshots/anim_scroll_before.png", Bitmap::ImageType::Png);

        window->Tree()->SetScroll(Vec2F(0.0f, 310.0f));
        for (int i = 0; i < 3; i++)
        {
            root->Update(0.016f);
            root->UpdateChildren(0.016f);
            root->UpdateChildrenTransforms();
        }

        ExpectRowControls(window->Tree(), "after");
        auto after = DrawRootAndCapture();
        ASSERT_TRUE(after);
        after->Save("TestScreenshots/anim_scroll_after.png", Bitmap::ImageType::Png);
    }
}
