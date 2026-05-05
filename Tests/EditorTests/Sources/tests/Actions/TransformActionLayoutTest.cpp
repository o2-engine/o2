#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// FrameTool's anchor handles now route through TransformAction (AppendAnchorsStep)
// instead of mutating SetLayout/SetTransform directly. The cascade itself only
// matters for Widgets (which need full UI init and live in the rendered test
// tier), but the data path — capturing a Layout payload in the action's before/
// done snapshots and ferrying it through Append/Redo/Undo — runs the same way
// for any SceneEditableObject. These tests pin that data path.

TEST(TransformActionLayout, AppendCarriesLayoutFromStepIntoMain)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({ a }));

    Layout newLayout(Vec2F(0.25f, 0.25f), Vec2F(0.75f, 0.75f),
                     Vec2F(5.0f, 6.0f), Vec2F(-5.0f, -6.0f));

    auto step = mmake<TransformAction>(AsEditable({ a }));
    step->doneTransforms = step->beforeTransforms;
    step->doneTransforms[0].layout = newLayout;
    main->Append(step);

    ASSERT_EQ(main->doneTransforms.Count(), 1);
    EXPECT_TRUE(main->doneTransforms[0].layout == newLayout)
        << "TryMerge must copy the step's Layout payload into main's done snapshot — "
           "the field FrameTool's AppendAnchorsStep relies on to record an anchor change.";
}

TEST(TransformActionLayout, CoalescedLayoutStepsKeepBeforeFrozen)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));

    auto main = mmake<TransformAction>(AsEditable({ a }));
    Layout originalBefore = main->beforeTransforms[0].layout;

    auto pushLayout = [&](const Layout& L)
    {
        auto step = mmake<TransformAction>(AsEditable({ a }));
        step->doneTransforms = step->beforeTransforms;
        step->doneTransforms[0].layout = L;
        main->Append(step);
    };

    pushLayout(Layout(Vec2F(0.1f, 0.1f), Vec2F(0.9f, 0.9f), Vec2F(), Vec2F()));
    pushLayout(Layout(Vec2F(0.2f, 0.2f), Vec2F(0.8f, 0.8f), Vec2F(), Vec2F()));
    Layout finalLayout(Vec2F(0.3f, 0.3f), Vec2F(0.7f, 0.7f), Vec2F(1.0f, 1.0f), Vec2F(-1.0f, -1.0f));
    pushLayout(finalLayout);

    EXPECT_TRUE(main->beforeTransforms[0].layout == originalBefore)
        << "Before-snapshot must stay pinned across coalesced steps so a single Undo "
           "restores the pre-drag layout, not the previous step's intermediate state.";
    EXPECT_TRUE(main->doneTransforms[0].layout == finalLayout)
        << "Done-snapshot must hold the latest layout from the merged span.";
}
