#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationVec3FTrack.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/KeyFramesTrackControl.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/MapKeyFramesTrackControl.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/DockableWindow.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct AnimationWindowProbe: AnimationWindow
    {
        AnimationWindowProbe(RefCounter* refCounter): AnimationWindow(refCounter) {}

        const Ref<DockableWindow>& Window() const { return mWindow; }
        const Ref<AnimationTree>& Tree() const { return mTree; }

        void SetClip(const Ref<AnimationClip>& clip)
        {
            mAnimation = clip;
            mHandlesSheet->SetAnimation(clip);
            mTimeline->SetAnimation(clip);
            mTree->SetAnimation(clip);
        }
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
}

// Keys of a Vec3F track (bone position/eulerAngles/scale) must show up in the animation window:
// the leaf node builds a keyframes track control and the collapsed parent nodes map its keys
TEST(AnimationWindowVec3FTrackUI, TrackShowsKeysOnTimeline)
{
    auto uiRoot = mmake<UIRoot>();

    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<Vec3F>("child/Bone/transform/position");
    track->AddKey(0.0f, Vec3F(0.0f, 0.0f, 0.0f));
    track->AddKey(1.0f, Vec3F(1.0f, 2.0f, 3.0f));

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        // The layout goes first: SetAnimation expands the tree, which realizes node widgets
        window->SetClip(clip);

        window->Window()->Update(0.016f);
        root->UpdateChildrenTransforms();

        Vector<Ref<AnimationTreeNode>> nodes;
        CollectWidgets(DynamicCast<Widget>(window->Tree()), nodes);
        ASSERT_FALSE(nodes.IsEmpty()) << "animation tree must realize node widgets";

        Ref<KeyFramesTrackControl<AnimationTrack<Vec3F>>> leafControl;
        Vector<Ref<MapKeyFramesTrackControl>> mapControls;
        for (auto& node : nodes)
        {
            Vector<Ref<KeyFramesTrackControl<AnimationTrack<Vec3F>>>> leafControls;
            CollectWidgets<KeyFramesTrackControl<AnimationTrack<Vec3F>>>(node, leafControls);
            if (!leafControls.IsEmpty())
                leafControl = leafControls[0];

            CollectWidgets<MapKeyFramesTrackControl>(node, mapControls);
        }

        ASSERT_NE(leafControl, nullptr) << "Vec3F track must build a key frames track control";
        EXPECT_EQ(leafControl->GetKeyHandles().Count(), 2) << "both keys must be shown on the leaf track";

        auto handlePositions = leafControl->GetKeyHandles()
            .Convert<float>([](auto& x) { return x->handle->GetPosition().x; })
            .Sorted([](float a, float b) { return a < b; });
        EXPECT_NEAR(handlePositions[0], 0.0f, 1e-4f);
        EXPECT_NEAR(handlePositions[1], 1.0f, 1e-4f);

        ASSERT_FALSE(mapControls.IsEmpty()) << "collapsed parent nodes must build mapped track controls";
        for (auto& mapControl : mapControls)
        {
            EXPECT_EQ(mapControl->GetKeyHandles().Count(), 2)
                << "mapped parent node must mirror the Vec3F track keys";
        }
    }
}
